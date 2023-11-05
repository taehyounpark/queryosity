#pragma once

#include "dataflow.h"
#include "lazy.h"
#include "lazy_varied.h"
#include "systematic_lookup.h"

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
 * @brief Node representing a operation to be performed in an analysis.
 * @details A delayed operation is not yet fully-specified to be considered
 * lazy, as they require existing lazy operations as inputs to create a lazy one
 * of itself.
 * @tparam T Input dataset type.
 * @tparam U Action for which a lazy one will be created.
 */
template <typename Bld>
class delayed : public systematic::lookup<delayed<Bld>>,
                public lockstep::node<Bld> {

public:
  class varied;

public:
  delayed(dataflow &dataflow, lockstep::node<Bld> &&operation)
      : systematic::lookup<delayed<Bld>>::lookup(dataflow),
        lockstep::node<Bld>::node(std::move(operation)) {}

  virtual ~delayed() = default;

  template <typename V>
  delayed(delayed<V> &&other)
      : systematic::lookup<delayed<Bld>>::lookup(*other.m_df),
        lockstep::node<Bld>::node(std::move(other)) {}

  template <typename V> delayed &operator=(delayed<V> &&other) {
    this->m_df = other.m_df;
    lockstep::node<Bld>::operator=(std::move(other));
    return *this;
  }

  virtual void set_variation(const std::string &var_name,
                             delayed &&var) override;

  virtual delayed const &nominal() const override;
  virtual delayed const &variation(const std::string &var_name) const override;

  virtual bool has_variation(const std::string &var_name) const override;
  virtual std::set<std::string> list_variation_names() const override;

  // template <
  //     typename... Args, typename V = Bld,
  //     std::enable_if_t<ana::column::template is_evaluator_v<V>, bool> =
  //     false>
  // auto vary(const std::string &var_name, Args &&...args) ->
  //     typename delayed<V>::varied;

  /**
   * @brief Evaluate the column out of existing ones.
   * @param columns Input columns.
   * @return Evaluated column.
   */
  template <
      typename... Nodes, typename V = Bld,
      std::enable_if_t<ana::column::template is_evaluator_v<V>, bool> = false>
  auto evaluate(Nodes &&...columns) const
      -> decltype(std::declval<delayed<V>>().evaluate_column(
          std::forward<Nodes>(columns)...)) {
    return this->evaluate_column(std::forward<Nodes>(columns)...);
  }

  template <typename... Nodes, typename V = Bld,
            std::enable_if_t<ana::column::template is_evaluator_v<V> &&
                                 ana::has_no_variation_v<Nodes...>,
                             bool> = false>
  auto evaluate_column(Nodes const &...columns) const
      -> lazy<column::template evaluated_t<V>> {
    // nominal
    return this->m_df->evaluate_column(*this, columns...);
  }

  template <typename... Nodes, typename V = Bld,
            std::enable_if_t<ana::column::template is_evaluator_v<V> &&
                                 ana::has_variation_v<Nodes...>,
                             bool> = false>
  auto evaluate_column(Nodes const &...columns) const ->
      typename lazy<column::template evaluated_t<V>>::varied {

    using varied_type = typename lazy<column::template evaluated_t<V>>::varied;

    auto nom = this->m_df->evaluate_column(*this, columns.nominal()...);
    auto syst = varied_type(std::move(nom));

    for (auto const &var_name : list_all_variation_names(columns...)) {
      auto var =
          this->m_df->evaluate_column(*this, columns.variation(var_name)...);
      syst.set_variation(var_name, std::move(var));
    }

    return syst;
  }

  /**
   * @brief Apply the selection's expression based on input columns.
   * @param columns Input columns.
   * @return Applied selection.
   */
  template <typename... Nodes, typename V = Bld,
            std::enable_if_t<ana::selection::template is_applicator_v<V>,
                             bool> = false>
  auto apply(Nodes &&...columns) const
      -> decltype(std::declval<delayed<V>>().apply_selection(
          std::forward<Nodes>(columns)...)) {
    return this->apply_selection(std::forward<Nodes>(columns)...);
  }

  /**
   * @brief Fill the aggregation with input columns.
   * @param columns Input columns
   * @return The aggregation filled with input columns.
   */
  template <
      typename... Nodes, typename V = Bld,
      std::enable_if_t<ana::aggregation::template is_booker_v<V>, bool> = false>
  auto fill(Nodes &&...columns) const
      -> decltype(std::declval<delayed<V>>().fill_aggregation(
          std::declval<Nodes>()...)) {
    return this->fill_aggregation(std::forward<Nodes>(columns)...);
  }

  /**
   * @brief Book the aggregation at a selection.
   * @param selection Selection to be counted.
   * @return The aggregation booked at the selection.
   */
  template <typename Node> auto book(Node &&selection) const {
    return this->select_aggregation(std::forward<Node>(selection));
  }

  template <typename Node, typename V = Bld,
            std::enable_if_t<ana::aggregation::template is_booker_v<V> &&
                                 ana::is_nominal_v<Node>,
                             bool> = false>
  auto select_aggregation(Node const &sel) const
      -> lazy<aggregation::booked_t<V>> {
    // nominal
    return this->m_df->select_aggregation(*this, sel);
  }

  template <typename Node, typename V = Bld,
            std::enable_if_t<ana::aggregation::template is_booker_v<V> &&
                                 ana::is_varied_v<Node>,
                             bool> = false>
  auto select_aggregation(Node const &sel) const ->
      typename lazy<aggregation::booked_t<V>>::varied {
    using varied_type = typename lazy<aggregation::booked_t<V>>::varied;
    auto syst =
        varied_type(this->m_df->select_aggregation(*this, sel.nominal()));
    for (auto const &var_name : list_all_variation_names(sel)) {
      syst.set_variation(var_name, this->m_df->select_aggregation(
                                       *this, sel.variation(var_name)));
    }
    return syst;
  }

  template <typename... Nodes> auto book(Nodes &&...nodes) const {
    static_assert(aggregation::template is_booker_v<Bld>,
                  "not a aggregation (booker)");
    return this->select_aggregations(std::forward<Nodes>(nodes)...);
  }

  template <typename... Nodes, typename V = Bld,
            std::enable_if_t<ana::aggregation::template is_booker_v<V> &&
                                 ana::has_no_variation_v<Nodes...>,
                             bool> = false>
  auto select_aggregations(Nodes const &...sels) const
      -> delayed<aggregation::bookkeeper<aggregation::booked_t<V>>> {
    // nominal
    return this->m_df->select_aggregations(*this, sels...);
  }

  template <typename... Nodes, typename V = Bld,
            std::enable_if_t<ana::aggregation::template is_booker_v<V> &&
                                 has_variation_v<Nodes...>,
                             bool> = false>
  auto select_aggregations(Nodes const &...sels) const -> typename delayed<
      aggregation::bookkeeper<aggregation::booked_t<V>>>::varied {
    // variations
    using varied_type = typename delayed<
        aggregation::bookkeeper<aggregation::booked_t<V>>>::varied;
    auto syst =
        varied_type(this->m_df->select_aggregations(*this, sels.nominal()...));
    for (auto const &var_name : list_all_variation_names(sels...)) {
      syst.set_variation(var_name, this->m_df->select_aggregations(
                                       *this, sels.variation(var_name)...));
    }
    return syst;
  }

  /**
   * @return The list of booked selection paths.
   */
  template <typename V = Bld,
            std::enable_if_t<ana::aggregation::template is_bookkeeper_v<V>,
                             bool> = false>
  auto list_selection_paths() const -> std::set<std::string> {
    return this->get_model_value(
        [](Bld const &bkpr) { return bkpr.list_selection_paths(); });
  }

  /**
   * @brief Shorthand for `evaluate()` and `apply()`
   * for column and selection respectively.
   * @param columns The input columns.
   * @return The evaluated/applied column/selection.
   */
  template <typename... Args, typename V = Bld,
            std::enable_if_t<column::template is_evaluator_v<V> ||
                                 selection::template is_applicator_v<V>,
                             bool> = false>
  auto operator()(Args &&...columns) const
      -> decltype(std::declval<delayed<V>>().evaluate_or_apply(
          std::forward<Args>(std::declval<Args &&>())...)) {
    return this->evaluate_or_apply(std::forward<Args>(columns)...);
  }

  /**
   * @brief Access a aggregation booked at a selection path.
   * @param selection_path The selection path.
   * @return The aggregation.
   */
  template <
      typename... Args, typename V = Bld,
      std::enable_if_t<aggregation::template is_bookkeeper_v<V>, bool> = false>
  auto operator[](const std::string &selection_path) const
      -> lazy<aggregation::booked_t<V>> {
    return this->get_aggregation(selection_path);
  }

protected:
  template <typename V = Bld,
            std::enable_if_t<ana::aggregation::template is_bookkeeper_v<V>,
                             bool> = false>
  auto get_aggregation(const std::string &selection_path) const
      -> lazy<aggregation::booked_t<V>> {
    return lazy<aggregation::booked_t<V>>(
        *this->m_df, lockstep::invoke_view(
                         [selection_path = selection_path](V &bkpr) {
                           return bkpr.get_aggregation(selection_path);
                         },
                         this->get_view()));
  }

  template <typename... Args, typename V = Bld,
            std::enable_if_t<column::template is_evaluator_v<V>, bool> = false>
  auto evaluate_or_apply(Args &&...columns) const
      -> decltype(std::declval<delayed<V>>().evaluate(
          std::forward<Args>(std::declval<Args &&>())...)) {
    return this->evaluate(std::forward<Args>(columns)...);
  }

  template <
      typename... Args, typename V = Bld,
      std::enable_if_t<selection::template is_applicator_v<V>, bool> = false>
  auto evaluate_or_apply(Args &&...columns) const
      -> decltype(std::declval<delayed<V>>().apply(
          std::forward<Args>(std::declval<Args &&>())...)) {
    return this->apply(std::forward<Args>(columns)...);
  }

  template <typename... Nodes, typename V = Bld,
            std::enable_if_t<ana::aggregation::template is_booker_v<V> &&
                                 ana::has_no_variation_v<Nodes...>,
                             bool> = false>
  auto fill_aggregation(Nodes const &...columns) const -> delayed<V> {
    // nominal
    return delayed<V>(
        *this->m_df,
        lockstep::invoke_node(
            [](V &fillable, typename Nodes::operation_type &...cols) {
              return fillable.book_fill(cols...);
            },
            this->get_view(), columns...));
  }

  template <typename... Nodes, typename V = Bld,
            std::enable_if_t<ana::aggregation::template is_booker_v<V> &&
                                 has_variation_v<Nodes...>,
                             bool> = false>
  auto fill_aggregation(Nodes const &...columns) const -> varied {
    auto syst = varied(std::move(this->fill_aggregation(columns.nominal()...)));
    for (auto const &var_name : list_all_variation_names(columns...)) {
      syst.set_variation(var_name, std::move(this->fill_aggregation(
                                       columns.variation(var_name)...)));
    }
    return syst;
  }

  template <typename... Nodes, typename V = Bld,
            std::enable_if_t<selection::template is_applicator_v<V> &&
                                 ana::has_no_variation_v<Nodes...>,
                             bool> = false>
  auto apply_selection(Nodes const &...columns) const -> lazy<selection> {
    // nominal
    return this->m_df->apply_selection(*this, columns...);
  }

  template <typename... Nodes, typename V = Bld,
            std::enable_if_t<selection::template is_applicator_v<V> &&
                                 ana::has_variation_v<Nodes...>,
                             bool> = false>
  auto apply_selection(Nodes const &...columns) const ->
      typename lazy<selection>::varied {
    // variations
    using varied_type = typename lazy<selection>::varied;
    auto syst =
        varied_type(this->nominal().apply_selection(columns.nominal()...));
    auto var_names = list_all_variation_names(columns...);
    for (auto const &var_name : var_names) {
      syst.set_variation(var_name, this->variation(var_name).apply_selection(
                                       columns.variation(var_name)...));
    }
    return syst;
  }
};

} // namespace ana

template <typename Bld>
void ana::delayed<Bld>::set_variation(const std::string &, delayed<Bld> &&) {
  // should never be called
  throw std::logic_error("cannot set variation to a lazy operation");
}

template <typename Bld>
auto ana::delayed<Bld>::nominal() const -> delayed const & {
  // this is nominal
  return *this;
}

template <typename Bld>
auto ana::delayed<Bld>::variation(const std::string &) const
    -> delayed const & {
  // propagation of variations must occur "transparently"
  return *this;
}

template <typename Bld>
std::set<std::string> ana::delayed<Bld>::list_variation_names() const {
  // no variations to list
  return std::set<std::string>();
}

template <typename Bld>
bool ana::delayed<Bld>::has_variation(const std::string &) const {
  // always false
  return false;
}

// template <typename Bld>
// template <typename... Args, typename V,
//           std::enable_if_t<ana::column::template is_evaluator_v<V>, bool>>
// auto ana::delayed<Bld>::vary(const std::string &var_name,
//                                        Args &&...args) ->
//     typename delayed<V>::varied {
//   auto syst = varied(std::move(*this));
//   syst.set_variation(var_name,
//                      std::move(syst.m_df->vary_evaluator(
//                          syst.nominal(), std::forward<Args>(args)...)));
//   return syst;
// }