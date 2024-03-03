#pragma once

#include "dataflow.h"
#include "lazy.h"
#include "lazy_varied.h"
#include "systematic.h"
#include "systematic_resolver.h"

namespace queryosity {

template <typename T> class todo;

template <typename U>
static constexpr std::true_type check_lazy(lazy<U> const &);
static constexpr std::false_type check_lazy(...);
template <typename U>
static constexpr std::true_type check_todo(todo<U> const &);
static constexpr std::false_type check_todo(...);

template <typename V>
static constexpr bool is_nominal_v =
    (decltype(check_lazy(std::declval<V>()))::value ||
     decltype(check_todo(std::declval<V>()))::value);
template <typename V> static constexpr bool is_varied_v = !is_nominal_v<V>;

template <typename... Args>
static constexpr bool has_no_variation_v = (is_nominal_v<Args> && ...);
template <typename... Args>
static constexpr bool has_variation_v = (is_varied_v<Args> || ...);

/**
 * @brief A node that instantiates a lazy action.
 * @details A todo node requires additional inputs to instantiate a lazy
 * action.
 * @tparam Bkr Booker that instantiates a lazy action.
 */
template <typename Bkr>
class todo : public dataflow::node,
             public concurrent::slotted<Bkr>,
             public systematic::resolver<todo<Bkr>> {

public:
  class varied;

public:
  todo(dataflow &df, std::vector<std::unique_ptr<Bkr>> bkr)
      : dataflow::node(df), m_slots(std::move(bkr)) {}

  virtual ~todo() = default;

  todo(todo &&) = default;
  todo &operator=(todo &&) = default;

  virtual Bkr *get_slot(unsigned int islot) const override;
  virtual unsigned int concurrency() const override;

  virtual void set_variation(const std::string &var_name, todo var) override;

  virtual todo &nominal() override;
  virtual todo &variation(const std::string &var_name) override;
  virtual todo const &nominal() const override;
  virtual todo const &variation(const std::string &var_name) const override;

  virtual bool has_variation(const std::string &var_name) const override;
  virtual std::set<std::string> list_variation_names() const override;

  /**
   * @brief Evaluate the column out of existing ones.
   * @param columns Input columns.
   * @return Evaluated column.
   */
  template <typename... Nodes, typename V = Bkr,
            std::enable_if_t<queryosity::column::template is_evaluatable_v<V>,
                             bool> = false>
  auto evaluate(Nodes &&...columns) const
      -> decltype(std::declval<todo<V>>()._evaluate(
          std::forward<Nodes>(columns)...)) {
    return this->_evaluate(std::forward<Nodes>(columns)...);
  }

  /**
   * @brief Fill the query with input columns.
   * @param columns Input columns
   * @return The query filled with input columns.
   */
  template <typename... Nodes, typename V = Bkr,
            std::enable_if_t<queryosity::query::template is_bookable_v<V>,
                             bool> = false>
  auto fill(Nodes &&...columns) const
      -> decltype(std::declval<todo<V>>().fill_query(
          std::declval<Nodes>()...)) {
    return this->fill_query(std::forward<Nodes>(columns)...);
  }

  /**
   * @brief Book the query at a selection.
   * @param selection Selection to be counted.
   * @return The query booked at the selection.
   */
  template <typename Node> auto book(Node &&selection) const {
    return this->_book(std::forward<Node>(selection));
  }

  template <typename... Nodes> auto book(Nodes &&...nodes) const {
    static_assert(query::template is_bookable_v<Bkr>, "not a query (book)");
    return this->_book(std::forward<Nodes>(nodes)...);
  }

  /**
   * @brief Shorthand for `evaluate()` and `apply()`
   * for column and selection respectively.
   * @param columns The input columns.
   * @return The evaluated/applied column/selection.
   */
  template <typename... Args, typename V = Bkr,
            std::enable_if_t<column::template is_evaluatable_v<V> ||
                                 selection::template is_applicable_v<V>,
                             bool> = false>
  auto operator()(Args &&...columns) const
      -> decltype(std::declval<todo<V>>().evaluate(
          std::forward<Args>(std::declval<Args &&>())...)) {
    return this->evaluate(std::forward<Args>(columns)...);
  }

protected:
  /**
   * @brief Evaluate a column definition out of nominal input columns
   */
  template <typename... Nodes, typename V = Bkr,
            std::enable_if_t<queryosity::column::template is_evaluatable_v<V> &&
                                 queryosity::has_no_variation_v<Nodes...>,
                             bool> = false>
  auto _evaluate(Nodes const &...columns) const
      -> lazy<column::template evaluated_t<V>> {
    return this->m_df->_evaluate(*this, columns...);
  }

  /**
   * @brief Evaluate a column definition out of at least one varied input
   * columns
   */
  template <typename... Nodes, typename V = Bkr,
            std::enable_if_t<queryosity::column::template is_evaluatable_v<V> &&
                                 queryosity::has_variation_v<Nodes...>,
                             bool> = false>
  auto _evaluate(Nodes const &...columns) const ->
      typename lazy<column::template evaluated_t<V>>::varied {

    using varied_type = typename lazy<column::template evaluated_t<V>>::varied;

    auto nom = this->m_df->_evaluate(this, columns.nominal()...);
    auto syst = varied_type(std::move(nom));

    for (auto const &var_name :
         systematic::list_all_variation_names(columns...)) {
      auto var = this->m_df->_evaluate(*this, columns.variation(var_name)...);
      syst.set_variation(var_name, std::move(var));
    }

    return syst;
  }

  /**
   * @brief Book an query at a nominal selection
   */
  template <typename Node, typename V = Bkr,
            std::enable_if_t<queryosity::query::template is_bookable_v<V> &&
                                 queryosity::is_nominal_v<Node>,
                             bool> = false>
  auto _book(Node const &sel) const -> lazy<query::booked_t<V>> {
    // nominal
    return this->m_df->_book(*this, sel);
  }

  /**
   * @brief Book an query at a varied selection
   */
  template <typename Node, typename V = Bkr,
            std::enable_if_t<queryosity::query::template is_bookable_v<V> &&
                                 queryosity::is_varied_v<Node>,
                             bool> = false>
  auto _book(Node const &sel) const ->
      typename lazy<query::booked_t<V>>::varied {
    using varied_type = typename lazy<query::booked_t<V>>::varied;
    auto syst = varied_type(this->m_df->_book(*this, sel.nominal()));
    for (auto const &var_name : systematic::list_all_variation_names(sel)) {
      syst.set_variation(var_name,
                         this->m_df->_book(*this, sel.variation(var_name)));
    }
    return syst;
  }

  template <typename... Nodes, typename V = Bkr,
            std::enable_if_t<queryosity::query::template is_bookable_v<V> &&
                                 queryosity::has_no_variation_v<Nodes...>,
                             bool> = false>
  auto _book(Nodes const &...sels) const
      -> std::array<lazy<query::booked_t<V>>, sizeof...(Nodes)> {
    // nominal
    return std::array<lazy<query::booked_t<V>>, sizeof...(Nodes)>{
        this->m_df->_book(*this, sels)...};
  }

  template <typename... Nodes, typename V = Bkr,
            std::enable_if_t<queryosity::query::template is_bookable_v<V> &&
                                 has_variation_v<Nodes...>,
                             bool> = false>
  auto _book(Nodes const &...sels) const
      -> std::array<typename lazy<query::booked_t<V>>::varied,
                    sizeof...(Nodes)> {
    // variations
    using varied_type = typename lazy<query::booked_t<V>>::varied;
    using array_of_varied_type =
        std::array<typename lazy<query::booked_t<V>>::varied, sizeof...(Nodes)>;
    auto var_names = systematic::list_all_variation_names(sels...);
    auto _book_varied =
        [var_names,
         this](systematic::resolver<lazy<selection::node>> const &sel) {
          auto syst = varied_type(this->m_df->_book(*this, sel.nominal()));
          for (auto const &var_name : var_names) {
            syst.set_variation(
                var_name, this->m_df->_book(*this, sel.variation(var_name)));
          }
          return syst;
        };
    return array_of_varied_type{_book_varied(sels)...};
  }

  template <typename... Nodes, typename V = Bkr,
            std::enable_if_t<queryosity::query::template is_bookable_v<V> &&
                                 queryosity::has_no_variation_v<Nodes...>,
                             bool> = false>
  auto fill_query(Nodes const &...columns) const -> todo<V> {
    // nominal
    return todo<V>(*this->m_df,
                   concurrent::invoke(
                       [](V *fillable, typename Nodes::action_type *...cols) {
                         return fillable->book_fill(*cols...);
                       },
                       this->get_slots(), columns.get_slots()...));
  }

  template <typename... Nodes, typename V = Bkr,
            std::enable_if_t<queryosity::query::template is_bookable_v<V> &&
                                 has_variation_v<Nodes...>,
                             bool> = false>
  auto fill_query(Nodes const &...columns) const -> varied {
    auto syst = varied(std::move(this->fill_query(columns.nominal()...)));
    for (auto const &var_name :
         systematic::list_all_variation_names(columns...)) {
      syst.set_variation(var_name, std::move(this->fill_query(
                                       columns.variation(var_name)...)));
    }
    return syst;
  }

protected:
  std::vector<std::unique_ptr<Bkr>> m_slots;
};

} // namespace queryosity

template <typename Action>
Action *queryosity::todo<Action>::get_slot(unsigned int islot) const {
  return this->m_slots[islot].get();
}

template <typename Action>
unsigned int queryosity::todo<Action>::concurrency() const {
  return this->m_slots.size();
}

template <typename Bkr>
void queryosity::todo<Bkr>::set_variation(const std::string &, todo<Bkr>) {
  // should never be called
  throw std::logic_error("cannot set variation to a nominal-only action");
}

template <typename Bkr> auto queryosity::todo<Bkr>::nominal() -> todo & {
  // this is nominal
  return *this;
}

template <typename Bkr>
auto queryosity::todo<Bkr>::variation(const std::string &) -> todo & {
  // propagation of variations must occur "transparently"
  return *this;
}

template <typename Bkr>
auto queryosity::todo<Bkr>::nominal() const -> todo const & {
  // this is nominal
  return *this;
}

template <typename Bkr>
auto queryosity::todo<Bkr>::variation(const std::string &) const
    -> todo const & {
  // propagation of variations must occur "transparently"
  return *this;
}

template <typename Bkr>
std::set<std::string> queryosity::todo<Bkr>::list_variation_names() const {
  // no variations to list
  return std::set<std::string>();
}

template <typename Bkr>
bool queryosity::todo<Bkr>::has_variation(const std::string &) const {
  // always false
  return false;
}