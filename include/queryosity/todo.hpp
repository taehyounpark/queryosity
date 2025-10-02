#pragma once

#include "dataflow.hpp"
#include "detail.hpp"
#include "lazy.hpp"
#include "lazy_varied.hpp"
#include "systematic.hpp"
#include "systematic_resolver.hpp"

namespace queryosity {

/**
 * @ingroup api
 * @brief Complete the instantiation of a lazy action.
 * @details A todo node is an intermediate state between the dataflow graph and
 * a lazy node, when additional methods must be chained in order to instantiate
 * the action.
 * @tparam Helper Helper class to instantiate the lazy action.
 */
template <typename Helper>
class todo : public dataflow::node,
             public ensemble::slotted<Helper>,
             public systematic::resolver<todo<Helper>> {

public:
  todo(dataflow &df, std::vector<std::unique_ptr<Helper>> bkr);
  virtual ~todo() = default;

  todo(todo &&) = default;
  todo &operator=(todo &&) = default;

  virtual std::vector<Helper *> const &get_slots() const final override;

  virtual void set_variation(const std::string &var_name,
                             todo var) final override;

  virtual todo &nominal() final override;
  virtual todo &variation(const std::string &var_name) final override;
  virtual todo const &nominal() const final override;
  virtual todo const &
  variation(const std::string &var_name) const final override;

  virtual bool has_variation(const std::string &var_name) const final override;
  virtual std::set<std::string> get_variation_names() const final override;

protected:
  template <typename... Nodes, typename V = Helper,
            std::enable_if_t<queryosity::column::is_evaluatable_v<V> &&
                                 queryosity::has_no_variation_v<Nodes...>,
                             bool> = false>
  auto _evaluate(Nodes const &...columns) const
      -> lazy<column::evaluated_t<V>> {
    return this->m_df->_evaluate(*this, columns...);
  }

  template <typename... Nodes, typename V = Helper,
            std::enable_if_t<queryosity::column::is_evaluatable_v<V> &&
                                 queryosity::has_variation_v<Nodes...>,
                             bool> = false>
  auto _evaluate(Nodes const &...columns) const
      -> varied<lazy<column::evaluated_t<V>>> {

    using varied_type =
        varied<lazy<column::evaluated_t<V>>>;

    auto nom = this->m_df->_evaluate(*this, columns.nominal()...);
    auto sys = varied_type(std::move(nom));

    for (auto const &var_name : systematic::get_variation_names(columns...)) {
      auto var = this->m_df->_evaluate(*this, columns.variation(var_name)...);
      sys.set_variation(var_name, std::move(var));
    }

    return sys;
  }

  template <typename... Nodes, typename V = Helper,
            std::enable_if_t<queryosity::selection::is_applicable_v<V> &&
                                 queryosity::has_no_variation_v<Nodes...>,
                             bool> = false>
  auto _apply(Nodes const &...columns) const -> lazy<selection::node> {
    using selection_type = typename V::selection_type;
    return this->m_df->template _apply<selection_type>(*this, columns...);
  }

  template <typename... Nodes, typename V = Helper,
            std::enable_if_t<queryosity::selection::is_applicable_v<V> &&
                                 queryosity::has_variation_v<Nodes...>,
                             bool> = false>
  auto _apply(Nodes const &...columns) const -> varied<lazy<selection::node>> {

    using selection_type = typename V::selection_type;
    using varied_type = varied<lazy<selection::node>>;

    auto nom = this->m_df->template _apply<selection_type>(
        *this, columns.nominal()...);
    auto sys = varied_type(nom);

    for (auto const &var_name : systematic::get_variation_names(columns...)) {
      auto var = this->m_df->template _apply<selection_type>(
          *this, columns.variation(var_name)...);
      sys.set_variation(var_name, var);
    }

    return sys;
  }

  template <typename Node, typename V = Helper,
            std::enable_if_t<queryosity::query::is_bookable_v<V> &&
                                 queryosity::is_nominal_v<Node>,
                             bool> = false>
  auto _book(Node const &sel) const -> lazy<query::booked_t<V>> {
    return this->m_df->_book(*this, sel);
  }

  template <typename Node, typename V = Helper,
            std::enable_if_t<queryosity::query::is_bookable_v<V> &&
                                 queryosity::is_varied_v<Node>,
                             bool> = false>
  auto _book(Node const &sel) const -> varied<lazy<query::booked_t<V>>> {
    using varied_type = varied<lazy<query::booked_t<V>>>;
    auto sys = varied_type(this->m_df->_book(*this, sel.nominal()));
    for (auto const &var_name : systematic::get_variation_names(sel)) {
      sys.set_variation(var_name,
                         this->m_df->_book(*this, sel.variation(var_name)));
    }
    return sys;
  }

  template <typename... Nodes, typename V = Helper,
            std::enable_if_t<queryosity::query::is_bookable_v<V> &&
                                 queryosity::has_no_variation_v<Nodes...>,
                             bool> = false>
  auto _book(Nodes const &...sels) const
      -> std::array<lazy<query::booked_t<V>>, sizeof...(Nodes)> {
    return std::array<lazy<query::booked_t<V>>, sizeof...(Nodes)>{
        this->m_df->_book(*this, sels)...};
  }

  template <typename... Nodes, typename V = Helper,
            std::enable_if_t<queryosity::query::is_bookable_v<V> &&
                                 has_variation_v<Nodes...>,
                             bool> = false>
  auto _book(Nodes const &...sels) const
      -> std::array<varied<lazy<query::booked_t<V>>>, sizeof...(Nodes)> {
    using varied_type = varied<lazy<query::booked_t<V>>>;
    using array_of_varied_type = std::array<varied_type, sizeof...(Nodes)>;
    auto var_names = systematic::get_variation_names(sels...);
    auto _book_varied =
        [var_names,
         this](systematic::resolver<lazy<selection::node>> const &sel) {
          auto sys = varied_type(this->m_df->_book(*this, sel.nominal()));
          for (auto const &var_name : var_names) {
            sys.set_variation(
                var_name, this->m_df->_book(*this, sel.variation(var_name)));
          }
          return sys;
        };
    return array_of_varied_type{_book_varied(sels)...};
  }

  template <typename... Nodes, typename V = Helper,
            std::enable_if_t<queryosity::query::is_fillable_v<query::booked_t<V>> &&
                                 queryosity::has_no_variation_v<Nodes...>,
                             bool> = false>
  auto _fill(Nodes const &...columns) const -> todo<V> {
    return todo<V>(*this->m_df,
                   ensemble::invoke(
                       [](V *fillable, typename Nodes::action_type *...cols) {
                         return fillable->add_columns(*cols...);
                       },
                       this->get_slots(), columns.get_slots()...));
  }

  template <typename... Nodes, typename V = Helper,
            std::enable_if_t<queryosity::query::is_fillable_v<query::booked_t<V>> &&
                                 has_variation_v<Nodes...>,
                             bool> = false>
  auto _fill(Nodes const &...columns) const -> varied<todo<Helper>> {
    using varied_type = varied<todo<V>>;
    auto sys = varied_type(std::move(this->_fill(columns.nominal()...)));
    for (auto const &var_name : systematic::get_variation_names(columns...)) {
      sys.set_variation(
          var_name, this->_fill(columns.variation(var_name)...));
    }
    return sys;
  }

public:

  /**
   * @brief Evaluate the column definition with input columns.
   * @param[in] columns Input columns.
   * @param[in][out] Evaluated column.
   */
  template <
      typename... Nodes, typename V = Helper,
      std::enable_if_t<queryosity::column::is_evaluatable_v<V>, bool> = false>
  auto evaluate(Nodes &&...columns) const
      -> decltype(std::declval<todo<V>>()._evaluate(
          std::forward<Nodes>(columns)...)) {
    return this->_evaluate(std::forward<Nodes>(columns)...);
  }

  /**
   * @brief Apply the selection with input columns.
   * @param[in] columns Input columns.
   * @param[in][out] Applied selection.
   */
  template <
      typename... Nodes, typename V = Helper,
      std::enable_if_t<queryosity::selection::is_applicable_v<V>, bool> = false>
  auto apply(Nodes &&...columns) const
      -> decltype(std::declval<todo<V>>()._apply(
          std::forward<Nodes>(columns)...)) {
    return this->template _apply(std::forward<Nodes>(columns)...);
  }

  /**
   * @brief Fill query with input columns per-entry.
   * @param[in] columns Input columns.
   * @returns Updated query plan filled with input columns.
   */
  template <typename... Nodes, typename V = Helper,
            std::enable_if_t<queryosity::query::is_fillable_v<query::booked_t<V>>, bool> = false>
  auto fill(Nodes &&...columns) const
      -> decltype(std::declval<todo<V>>()._fill(std::declval<Nodes>()...)) {
    return this->_fill(std::forward<Nodes>(columns)...);
  }

  /**
   * @brief Book a query at multiple selections.
   * @tparam Sels... Selections.
   * @param[in] sels... selection nodes.
   * @return `std::tuple` of queries booked at each selection.
   */
  template <typename... Sels> auto at(Sels &&...sels) const -> decltype(this->_book(std::forward<Sels>(sels)...)){
    static_assert(query::is_bookable_v<Helper>, "not bookable");
    return this->_book(std::forward<Sels>(sels)...);
  }

  /**
   * @brief Shorthand for `evaluate()`.
   * @tparam Args... Input column types.
   * @param[in] columns... Input columns.
   * @return Evaluated column.
   */
  template <typename... Args> auto operator()(Args &&...columns) const {
    if constexpr (column::is_evaluatable_v<Helper>) {
      return this->evaluate(std::forward<Args>(columns)...);
    } else if constexpr (selection::is_applicable_v<Helper>) {
      return this->apply(std::forward<Args>(columns)...);
    } else if constexpr (query::is_bookable_v<Helper>) {
      return this->fill(std::forward<Args>(columns)...);
    }
  }

  template <typename Node>
  auto fill(Node&& node) const
      -> decltype(this->_fill(std::forward<Node>(node))) {
      return this->_fill(std::forward<Node>(node));
  }

protected:
  std::vector<std::unique_ptr<Helper>> m_slots;
  std::vector<Helper *> m_ptrs;
};

} // namespace queryosity

template <typename Helper>
queryosity::todo<Helper>::todo(queryosity::dataflow &df,
                               std::vector<std::unique_ptr<Helper>> bkr)
    : dataflow::node(df), m_slots(std::move(bkr)) {
  m_ptrs.reserve(m_slots.size());
  for (auto const &slot : m_slots) {
    m_ptrs.push_back(slot.get());
  }
}

template <typename Helper>
std::vector<Helper *> const &queryosity::todo<Helper>::get_slots() const {
  return m_ptrs;
}

template <typename Helper>
void queryosity::todo<Helper>::set_variation(const std::string &,
                                             todo<Helper>) {
  // should never be called
  throw std::logic_error("cannot set variation to a nominal-only action");
}

template <typename Helper>
auto queryosity::todo<Helper>::nominal() -> todo<Helper> & {
  // this is nominal
  return *this;
}

template <typename Helper>
auto queryosity::todo<Helper>::variation(const std::string &)
    -> todo<Helper> & {
  // propagation of variations must occur "transparently"
  return *this;
}

template <typename Helper>
auto queryosity::todo<Helper>::nominal() const -> todo const & {
  // this is nominal
  return *this;
}

template <typename Helper>
auto queryosity::todo<Helper>::variation(const std::string &) const
    -> todo const & {
  // propagation of variations must occur "transparently"
  return *this;
}

template <typename Helper>
std::set<std::string> queryosity::todo<Helper>::get_variation_names() const {
  // no variations to list
  return std::set<std::string>();
}

template <typename Helper>
bool queryosity::todo<Helper>::has_variation(const std::string &) const {
  // always false
  return false;
}