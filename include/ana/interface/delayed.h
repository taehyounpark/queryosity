#pragma once

#include "concurrent.h"
#include "dataflow.h"
#include "lazy.h"

namespace ana {

template <typename DS>
template <typename Bld>
class dataflow<DS>::delayed : public dataflow<DS>::template node<delayed<Bld>>,
                              public concurrent<Bld> {

public:
  class varied;

public:
  using dataflow_type = typename node<delayed<Bld>>::dataflow_type;
  using dataset_type = typename node<delayed<Bld>>::dataset_type;
  using action_type = Bld;

  template <typename Sel, typename... Args>
  using lazy_selection_applicator_t =
      decltype(std::declval<dataflow<DS>>().template filter<Sel>(
          std::declval<std::string>(), std::declval<Args>()...));

  template <typename Sel, typename... Args>
  using selection_applicator_t =
      typename decltype(std::declval<dataflow<DS>>().template filter<Sel>(
          std::declval<std::string>(), std::declval<Args>()...))::action_type;

public:
  delayed(dataflow<DS> &dataflow, const concurrent<Bld> &action)
      : node<delayed<Bld>>::node(dataflow),
        concurrent<Bld>::concurrent(action) {}

  virtual ~delayed() = default;

  template <typename V>
  delayed(delayed<V> const &other)
      : node<delayed<Bld>>::node(*other.m_df),
        concurrent<Bld>::concurrent(other) {}

  template <typename V> delayed &operator=(delayed<V> const &other) {
    concurrent<Bld>::operator=(other);
    this->m_df = other.m_df;
    return *this;
  }

  virtual void set_variation(const std::string &var_name,
                             delayed const &var) override;

  virtual delayed get_nominal() const override;
  virtual delayed get_variation(const std::string &var_name) const override;

  virtual bool has_variation(const std::string &var_name) const override;
  virtual std::set<std::string> list_variation_names() const override;

  /**
   * @brief Apply a systematic variation to an `equation` column.
   * @param var_name Name of the systematic variation.
   * @param args... Constructor arguments for `definition`.
   * @return Varied definition.
   * @details Creates a `varied` action whose `.get_nominal()` is the original
   * lazy one, and `get_variation(var_name)` is the newly-constructed one.
   */
  // template <typename... Args, typename V = Bld,
  //           std::enable_if_t<ana::column::template is_evaluator_v<V> &&
  //                                !ana::column::template is_equation_v<
  //                                    ana::column::template evaluated_t<V>>,
  //                            bool> = false>
  // auto vary(const std::string &var_name, Args &&...args) -> varied<V>;

  /**
   * @brief Apply a systematic variation to `equation` column.
   * @param var_name Name of the systematic variation.
   * @param callable C++ function, lambda expression, or any other callable.
   * **Note**: the function return type and signature must be convertible to the
   * original's.
   * @return Varied equation.
   * @details Creates a `varied` action whose `.get_nominal()` is the original
   * lazy one, and `get_variation(var_name)` is the newly-constructed one.
   */
  // template <typename F, typename V = Bld,
  //           std::enable_if_t<ana::column::template is_evaluator_v<V> &&
  //                                ana::column::template is_equation_v<
  //                                    ana::column::template evaluated_t<V>>,
  //                            bool> = false>
  // auto vary(const std::string &var_name, F callable) -> varied<V>;

  /**
   * @brief Evaluate the column out of existing ones.
   * @param columns Input columns.
   * @return Evaluated column.
   * @details The input column(s) can be `lazy` or `varied`. Correspondingly,
   * the evaluated column will be as well.
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
            std::enable_if_t<
                ana::column::template is_evaluator_v<V> &&
                    ana::dataflow<DS>::template has_no_variation_v<Nodes...>,
                bool> = false>
  auto evaluate_column(Nodes const &...columns) const
      -> lazy<column::template evaluated_t<V>> {
    // nominal
    return this->m_df->evaluate_column(*this, columns...);
  }

  // template <
  //     typename... Nodes, typename V = Bld,
  //     std::enable_if_t<ana::column::template is_evaluator_v<V> &&
  //                          ana::dataflow<DS>::template
  //                          has_variation_v<Nodes...>,
  //                      bool> = false>
  // auto evaluate_column(Nodes const &...columns) const
  //     -> varied<column::template evaluated_t<V>> {
  //   // variations
  //   auto nom =
  //       this->m_df->evaluate_column(*this, columns.get_nominal()...);
  //   varied<column::template evaluated_t<V>> syst(nom);
  //   for (auto const &var_name : list_all_variation_names(columns...)) {
  //     auto var = this->m_df->evaluate_column(
  //         *this, columns.get_variation(var_name)...);
  //     syst.set_variation(var_name, var);
  //   }
  //   return syst;
  // }

  /**
   * @brief Apply the selection's expression based on input columns.
   * @param columns Input columns.
   * @return `lazy/varied<selection>` Applied selection.
   */
  template <typename... Nodes, typename V = Bld,
            std::enable_if_t<ana::selection::template is_applicator_v<V>,
                             bool> = false>
  auto apply(Nodes &&...columns) const
      -> decltype(std::declval<delayed<V>>().apply_selection(
          std::forward<Nodes>(columns)...)) {
    return this->apply_selection(std::forward<Nodes>(columns)...);
  }

  template <typename... Nodes, typename V = Bld,
            std::enable_if_t<
                selection::template is_applicator_v<V> &&
                    ana::dataflow<DS>::template has_no_variation_v<Nodes...>,
                bool> = false>
  auto apply_selection(Nodes const &...columns) const -> lazy<selection> {
    // nominal
    return this->m_df->apply_selection(*this, columns...);
  }

  // template <
  //     typename... Nodes, typename V = Bld,
  //     std::enable_if_t<selection::template is_applicator_v<V> &&
  //                          ana::dataflow<DS>::template
  //                          has_variation_v<Nodes...>,
  //                      bool> = false>
  // auto apply_selection(Nodes const &...columns) const -> varied<selection> {
  //   // variations
  //   varied<selection> syst(
  //       this->get_nominal().apply_selection(columns.get_nominal()...));
  //   auto var_names = list_all_variation_names(columns...);
  //   for (auto const &var_name : var_names) {
  //     syst.set_variation(var_name,
  //                        this->get_variation(var_name).apply_selection(
  //                            columns.get_variation(var_name)...));
  //   }
  //   return syst;
  // }

  /**
   * @brief Fill the counter with input columns.
   * @param columns Input (`lazy` or `varied`) columns.
   * @return The counter (`lazy` or `varied`) filled with the input columns.
   */
  template <
      typename... Nodes, typename V = Bld,
      std::enable_if_t<ana::counter::template is_booker_v<V>, bool> = false>
  auto fill(Nodes &&...columns) const
      -> decltype(std::declval<delayed<V>>().fill_counter(
          std::declval<Nodes>()...)) {
    return this->fill_counter(std::forward<Nodes>(columns)...);
  }

  template <typename... Nodes, typename V = Bld,
            std::enable_if_t<
                ana::counter::template is_booker_v<V> &&
                    ana::dataflow<DS>::template has_no_variation_v<Nodes...>,
                bool> = false>
  auto fill_counter(Nodes const &...columns) const -> delayed<V> {
    // nominal
    return delayed<V>(
        *this->m_df, this->get_concurrent_result(
                         [](V &fillable, typename Nodes::action_type &...cols) {
                           return fillable.book_fill(cols...);
                         },
                         columns...));
  }

  // template <
  //     typename... Nodes, typename V = Bld,
  //     std::enable_if_t<ana::counter::template is_booker_v<V> &&
  //                          ana::dataflow<DS>::template
  //                          has_variation_v<Nodes...>,
  //                      bool> = false>
  // auto fill_counter(Nodes const &...columns) const -> varied<V> {
  //   // variations
  //   auto syst = varied<V>(this->fill_counter(columns.get_nominal()...));
  //   for (auto const &var_name : list_all_variation_names(columns...)) {
  //     syst.set_variation(
  //         var_name, this->fill_counter(columns.get_variation(var_name)...));
  //   }
  //   return syst;
  // }

  /**
   * @brief Book the counter at a selection.
   * @param selection Selection to be counted.
   * @return The (`lazy` or `varied`) counter booked at the selection.
   */
  template <typename Node> auto at(Node &&selection) const {
    return this->book_selection(std::forward<Node>(selection));
  }

  template <typename Node, typename V = Bld,
            std::enable_if_t<ana::counter::template is_booker_v<V> &&
                                 ana::dataflow<DS>::template is_nominal_v<Node>,
                             bool> = false>
  auto book_selection(Node const &sel) const -> lazy<booked_counter_t<V>> {
    // nominal
    return this->m_df->book_selection(*this, sel);
  }

  // template <typename Node, typename V = Bld,
  //           std::enable_if_t<ana::counter::template is_booker_v<V> &&
  //                                ana::dataflow<DS>::template
  //                                is_varied_v<Node>,
  //                            bool> = false>
  // auto book_selection(Node const &sel) const -> varied<booked_counter_t<V>> {
  //   // variations
  //   auto syst = varied<booked_counter_t<V>>(
  //       this->m_df->book_selection(*this, sel.get_nominal()));
  //   for (auto const &var_name : list_all_variation_names(sel)) {
  //     syst.set_variation(var_name, this->m_df->book_selection(
  //                                      *this, sel.get_variation(var_name)));
  //   }
  //   return syst;
  // }

  /**
   * @brief Book the counter at multiple selections.
   * @param selections Selections to book the counter at.
   * @return The (`lazy` or `varied`) counter "booker" that keeps track the
   * counters at each selection.
   */
  template <typename... Nodes> auto at(Nodes &&...nodes) const {
    static_assert(counter::template is_booker_v<Bld>, "not a counter (booker)");
    return this->book_selections(std::forward<Nodes>(nodes)...);
  }

  template <typename... Nodes, typename V = Bld,
            std::enable_if_t<
                ana::counter::template is_booker_v<V> &&
                    ana::dataflow<DS>::template has_no_variation_v<Nodes...>,
                bool> = false>
  auto book_selections(Nodes const &...sels) const -> delayed<V> {
    // nominal
    return this->m_df->book_selections(*this, sels...);
  }

  // template <
  //     typename... Nodes, typename V = Bld,
  //     std::enable_if_t<ana::counter::template is_booker_v<V> &&
  //                          ana::dataflow<DS>::template
  //                          has_variation_v<Nodes...>,
  //                      bool> = false>
  // auto book_selections(Nodes const &...sels) const -> varied<V> {
  //   // variations
  //   auto syst = varied<V>(
  //       this->m_df->book_selections(*this, sels.get_nominal()...));
  //   for (auto const &var_name : list_all_variation_names(sels...)) {
  //     syst.set_variation(var_name, this->m_df->book_selections(
  //                                      *this,
  //                                      sels.get_variation(var_name)...));
  //   }
  //   return syst;
  // }

  /**
   * @return The list of booked selection paths.
   */
  template <
      typename V = Bld,
      std::enable_if_t<ana::counter::template is_booker_v<V>, bool> = false>
  auto list_selection_paths() const -> std::set<std::string> {
    return this->get_model_value(
        [](Bld const &bkr) { return bkr.list_selection_paths(); });
  }

  /**
   * @brief Get the counter booked at a selection path.
   * @param selection_path Path of the selection
   * @return counter The counter booked at the selection path.
   */
  template <
      typename V = Bld,
      std::enable_if_t<ana::counter::template is_booker_v<V>, bool> = false>
  auto get_counter(const std::string &sel_path) const
      -> lazy<booked_counter_t<V>> {
    return lazy<typename V::counter_type>(
        *this->m_df,
        this->get_concurrent_result([sel_path = sel_path](Bld &bkr) {
          return bkr.get_counter(sel_path);
        }));
  }

  /**
   * @brief Retrieve the result of a counter.
   * @details Triggers processing of the dataset if that the result is not
   * already available.
   * @return The result of the implemented counter.
   */
  template <typename V = Bld,
            std::enable_if_t<ana::counter::template is_implemented_v<V>, bool> =
                false>
  auto get_result() const -> decltype(std::declval<V>().get_result()) {
    this->m_df->analyze();
    this->merge_results();
    return this->get_model()->get_result();
  }

  /**
   * @brief Evaluate/apply a column/selection, respectively.
   * @details A chained function call is equivalent to `evaluate` and `apply`
   * for column and selection respectively.
   * @return The resulting (`lazy` or `varied`) counter/selection from its
   * evaluator/application.
   */
  template <typename... Args, typename V = Bld,
            std::enable_if_t<column::template is_evaluator_v<V> ||
                                 selection::template is_applicator_v<V>,
                             bool> = false>
  auto operator()(Args &&...columns)
      -> decltype(std::declval<delayed<V>>().evaluate_or_apply(
          std::forward<Args>(std::declval<Args &&>())...)) {
    return this->evaluate_or_apply(std::forward<Args>(columns)...);
  }

  template <typename... Args, typename V = Bld,
            std::enable_if_t<column::template is_evaluator_v<V>, bool> = false>
  auto evaluate_or_apply(Args &&...columns)
      -> decltype(std::declval<delayed<V>>().evaluate(
          std::forward<Args>(std::declval<Args &&>())...)) {
    return this->evaluate(std::forward<Args>(columns)...);
  }

  template <
      typename... Args, typename V = Bld,
      std::enable_if_t<selection::template is_applicator_v<V>, bool> = false>
  auto evaluate_or_apply(Args &&...columns)
      -> decltype(std::declval<delayed<V>>().apply(
          std::forward<Args>(std::declval<Args &&>())...)) {
    return this->apply(std::forward<Args>(columns)...);
  }
};

} // namespace ana

template <typename DS>
template <typename Bld>
void ana::dataflow<DS>::delayed<Bld>::set_variation(const std::string &,
                                                    delayed<Bld> const &) {
  // should never be called
  throw std::logic_error("cannot set variation to a lazy action");
}

template <typename DS>
template <typename Bld>
auto ana::dataflow<DS>::delayed<Bld>::get_nominal() const -> delayed<Bld> {
  // this is nominal
  return *this;
}

template <typename DS>
template <typename Bld>
auto ana::dataflow<DS>::delayed<Bld>::get_variation(const std::string &) const
    -> delayed<Bld> {
  // propagation of variations must occur "transparently"
  return typename dataflow<DS>::template delayed<Bld>(*this->m_df, *this);
}

template <typename DS>
template <typename Bld>
std::set<std::string>
ana::dataflow<DS>::delayed<Bld>::list_variation_names() const {
  // no variations to list
  return std::set<std::string>();
}

template <typename DS>
template <typename Bld>
bool ana::dataflow<DS>::delayed<Bld>::has_variation(const std::string &) const {
  // always false
  return false;
}