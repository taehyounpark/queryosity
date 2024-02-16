#pragma once

#include "dataflow.h"
#include "lazy.h"
#include "lazy_varied.h"
#include "systematic.h"
#include "systematic_resolver.h"

namespace ana {

template <typename T> class delayed;

template <typename U>
static constexpr std::true_type check_lazy(lazy<U> const &);
static constexpr std::false_type check_lazy(...);
template <typename U>
static constexpr std::true_type check_delayed(delayed<U> const &);
static constexpr std::false_type check_delayed(...);

template <typename V>
static constexpr bool is_nominal_v =
    (decltype(check_lazy(std::declval<V>()))::value ||
     decltype(check_delayed(std::declval<V>()))::value);
template <typename V> static constexpr bool is_varied_v = !is_nominal_v<V>;

template <typename... Args>
static constexpr bool has_no_variation_v = (is_nominal_v<Args> && ...);
template <typename... Args>
static constexpr bool has_variation_v = (is_varied_v<Args> || ...);

/**
 * @brief A node that instantiates a lazy action.
 * @details A delayed node requires additional inputs to instantiate a lazy
 * action.
 * @tparam Bkr Booker that instantiates a lazy action.
 */
template <typename Bkr>
class delayed : public systematic::resolver<delayed<Bkr>>,
                public lockstep::node<Bkr> {

public:
  class varied;

public:
  delayed(dataflow &dataflow, lockstep::node<Bkr> &&operation)
      : systematic::resolver<delayed<Bkr>>::resolver(dataflow),
        lockstep::node<Bkr>::node(std::move(operation)) {}

  virtual ~delayed() = default;

  template <typename V>
  delayed(delayed<V> &&other)
      : systematic::resolver<delayed<Bkr>>::resolver(*other.m_df),
        lockstep::node<Bkr>::node(std::move(other)) {}

  template <typename V> delayed &operator=(delayed<V> &&other) {
    this->m_df = other.m_df;
    lockstep::node<Bkr>::operator=(std::move(other));
    return *this;
  }

  virtual void set_variation(const std::string &var_name,
                             delayed &&var) override;

  virtual delayed const &nominal() const override;
  virtual delayed const &variation(const std::string &var_name) const override;

  virtual bool has_variation(const std::string &var_name) const override;
  virtual std::set<std::string> list_variation_names() const override;

  /**
   * @brief Evaluate the column out of existing ones.
   * @param columns Input columns.
   * @return Evaluated column.
   */
  template <
      typename... Nodes, typename V = Bkr,
      std::enable_if_t<ana::column::template is_evaluator_v<V>, bool> = false>
  auto evaluate(Nodes &&...columns) const
      -> decltype(std::declval<delayed<V>>().evaluate_column(
          std::forward<Nodes>(columns)...)) {
    return this->evaluate_column(std::forward<Nodes>(columns)...);
  }

  /**
   * @brief Apply the selection's expression based on input columns.
   * @param columns Input columns.
   * @return Applied selection.
   */
  template <typename... Nodes, typename V = Bkr,
            std::enable_if_t<ana::selection::template is_applicator_v<V>,
                             bool> = false>
  auto apply(Nodes &&...columns) const
      -> decltype(std::declval<delayed<V>>()._apply(
          std::forward<Nodes>(columns)...)) {
    return this->_apply(std::forward<Nodes>(columns)...);
  }

  /**
   * @brief Fill the counter with input columns.
   * @param columns Input columns
   * @return The counter filled with input columns.
   */
  template <
      typename... Nodes, typename V = Bkr,
      std::enable_if_t<ana::counter::template is_booker_v<V>, bool> = false>
  auto fill(Nodes &&...columns) const
      -> decltype(std::declval<delayed<V>>().fill_counter(
          std::declval<Nodes>()...)) {
    return this->fill_counter(std::forward<Nodes>(columns)...);
  }

  /**
   * @brief Book the counter at a selection.
   * @param selection Selection to be counted.
   * @return The counter booked at the selection.
   */
  template <typename Node> auto book(Node &&selection) const {
    return this->_book(std::forward<Node>(selection));
  }

  template <typename... Nodes> auto book(Nodes &&...nodes) const {
    static_assert(counter::template is_booker_v<Bkr>, "not a counter (booker)");
    return this->_book(std::forward<Nodes>(nodes)...);
  }

  /**
   * @brief Shorthand for `evaluate()` and `apply()`
   * for column and selection respectively.
   * @param columns The input columns.
   * @return The evaluated/applied column/selection.
   */
  template <typename... Args, typename V = Bkr,
            std::enable_if_t<column::template is_evaluator_v<V> ||
                                 selection::template is_applicator_v<V>,
                             bool> = false>
  auto operator()(Args &&...columns) const
      -> decltype(std::declval<delayed<V>>().evaluate_or_apply(
          std::forward<Args>(std::declval<Args &&>())...)) {
    return this->evaluate_or_apply(std::forward<Args>(columns)...);
  }

protected:
  /**
   * @brief Evaluate a column definition out of nominal input columns
   */
  template <typename... Nodes, typename V = Bkr,
            std::enable_if_t<ana::column::template is_evaluator_v<V> &&
                                 ana::has_no_variation_v<Nodes...>,
                             bool> = false>
  auto evaluate_column(Nodes const &...columns) const
      -> lazy<column::template evaluated_t<V>> {
    return this->m_df->evaluate_column(*this, columns...);
  }

  /**
   * @brief Evaluate a column definition out of at least one varied input
   * columns
   */
  template <typename... Nodes, typename V = Bkr,
            std::enable_if_t<ana::column::template is_evaluator_v<V> &&
                                 ana::has_variation_v<Nodes...>,
                             bool> = false>
  auto evaluate_column(Nodes const &...columns) const ->
      typename lazy<column::template evaluated_t<V>>::varied {

    using varied_type = typename lazy<column::template evaluated_t<V>>::varied;

    auto nom = this->m_df->evaluate_column(*this, columns.nominal()...);
    auto syst = varied_type(std::move(nom));

    for (auto const &var_name :
         systematic::list_all_variation_names(columns...)) {
      auto var =
          this->m_df->evaluate_column(*this, columns.variation(var_name)...);
      syst.set_variation(var_name, std::move(var));
    }

    return syst;
  }

  /**
   * @brief Book an counter at a nominal selection
   */
  template <typename Node, typename V = Bkr,
            std::enable_if_t<ana::counter::template is_booker_v<V> &&
                                 ana::is_nominal_v<Node>,
                             bool> = false>
  auto _book(Node const &sel) const -> lazy<counter::booked_t<V>> {
    // nominal
    return this->m_df->_book(*this, sel);
  }

  /**
   * @brief Book an counter at a varied selection
   */
  template <typename Node, typename V = Bkr,
            std::enable_if_t<ana::counter::template is_booker_v<V> &&
                                 ana::is_varied_v<Node>,
                             bool> = false>
  auto _book(Node const &sel) const ->
      typename lazy<counter::booked_t<V>>::varied {
    using varied_type = typename lazy<counter::booked_t<V>>::varied;
    auto syst = varied_type(this->m_df->_book(*this, sel.nominal()));
    for (auto const &var_name : systematic::list_all_variation_names(sel)) {
      syst.set_variation(var_name,
                         this->m_df->_book(*this, sel.variation(var_name)));
    }
    return syst;
  }

  template <typename... Nodes, typename V = Bkr,
            std::enable_if_t<ana::counter::template is_booker_v<V> &&
                                 ana::has_no_variation_v<Nodes...>,
                             bool> = false>
  auto _book(Nodes const &...sels) const
      -> std::array<lazy<counter::booked_t<V>>, sizeof...(Nodes)> {
    // nominal
    return std::array<lazy<counter::booked_t<V>>, sizeof...(Nodes)>{
        this->m_df->_book(*this, sels)...};
  }

  template <typename... Nodes, typename V = Bkr,
            std::enable_if_t<ana::counter::template is_booker_v<V> &&
                                 has_variation_v<Nodes...>,
                             bool> = false>
  auto _book(Nodes const &...sels) const
      -> std::array<typename lazy<counter::booked_t<V>>::varied,
                    sizeof...(Nodes)> {
    // variations
    using varied_type = typename lazy<counter::booked_t<V>>::varied;
    using array_of_varied_type =
        std::array<typename lazy<counter::booked_t<V>>::varied,
                   sizeof...(Nodes)>;
    auto var_names = systematic::list_all_variation_names(sels...);
    auto _book_varied = [var_names, this](
                            systematic::resolver<lazy<selection>> const &sel) {
      auto syst = varied_type(this->m_df->_book(*this, sel.nominal()));
      for (auto const &var_name : var_names) {
        syst.set_variation(var_name,
                           this->m_df->_book(*this, sel.variation(var_name)));
      }
      return syst;
    };
    return array_of_varied_type{_book_varied(sels)...};
  }

  template <typename... Args, typename V = Bkr,
            std::enable_if_t<column::template is_evaluator_v<V>, bool> = false>
  auto evaluate_or_apply(Args &&...columns) const
      -> decltype(std::declval<delayed<V>>().evaluate(
          std::forward<Args>(std::declval<Args &&>())...)) {
    return this->evaluate(std::forward<Args>(columns)...);
  }

  template <
      typename... Args, typename V = Bkr,
      std::enable_if_t<selection::template is_applicator_v<V>, bool> = false>
  auto evaluate_or_apply(Args &&...columns) const
      -> decltype(std::declval<delayed<V>>().apply(
          std::forward<Args>(std::declval<Args &&>())...)) {
    return this->apply(std::forward<Args>(columns)...);
  }

  template <typename... Nodes, typename V = Bkr,
            std::enable_if_t<ana::counter::template is_booker_v<V> &&
                                 ana::has_no_variation_v<Nodes...>,
                             bool> = false>
  auto fill_counter(Nodes const &...columns) const -> delayed<V> {
    // nominal
    return delayed<V>(
        *this->m_df,
        lockstep::get_node(
            [](V *fillable, typename Nodes::operation_type *...cols) {
              return fillable->book_fill(*cols...);
            },
            *this, columns...));
  }

  template <typename... Nodes, typename V = Bkr,
            std::enable_if_t<ana::counter::template is_booker_v<V> &&
                                 has_variation_v<Nodes...>,
                             bool> = false>
  auto fill_counter(Nodes const &...columns) const -> varied {
    auto syst = varied(std::move(this->fill_counter(columns.nominal()...)));
    for (auto const &var_name :
         systematic::list_all_variation_names(columns...)) {
      syst.set_variation(var_name, std::move(this->fill_counter(
                                       columns.variation(var_name)...)));
    }
    return syst;
  }

  template <typename... Nodes, typename V = Bkr,
            std::enable_if_t<selection::template is_applicator_v<V> &&
                                 ana::has_no_variation_v<Nodes...>,
                             bool> = false>
  auto _apply(Nodes const &...columns) const -> lazy<selection> {
    // nominal
    return this->m_df->_apply(*this, columns...);
  }

  template <typename... Nodes, typename V = Bkr,
            std::enable_if_t<selection::template is_applicator_v<V> &&
                                 ana::has_variation_v<Nodes...>,
                             bool> = false>
  auto _apply(Nodes const &...columns) const ->
      typename lazy<selection>::varied {
    // variations
    using varied_type = typename lazy<selection>::varied;
    auto syst = varied_type(this->nominal()._apply(columns.nominal()...));
    auto var_names = systematic::list_all_variation_names(columns...);
    for (auto const &var_name : var_names) {
      syst.set_variation(var_name, this->variation(var_name)._apply(
                                       columns.variation(var_name)...));
    }
    return syst;
  }
};

} // namespace ana

template <typename Bkr>
void ana::delayed<Bkr>::set_variation(const std::string &, delayed<Bkr> &&) {
  // should never be called
  throw std::logic_error("cannot set variation to a lazy operation");
}

template <typename Bkr>
auto ana::delayed<Bkr>::nominal() const -> delayed const & {
  // this is nominal
  return *this;
}

template <typename Bkr>
auto ana::delayed<Bkr>::variation(const std::string &) const
    -> delayed const & {
  // propagation of variations must occur "transparently"
  return *this;
}

template <typename Bkr>
std::set<std::string> ana::delayed<Bkr>::list_variation_names() const {
  // no variations to list
  return std::set<std::string>();
}

template <typename Bkr>
bool ana::delayed<Bkr>::has_variation(const std::string &) const {
  // always false
  return false;
}