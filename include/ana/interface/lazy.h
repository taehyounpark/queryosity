/**
 * @file
 * @author Tae Hyoun Park <taehyounpark@icloud.com>
 * @version 0.1 *
 */

#pragma once

#include <iostream>
#include <set>
#include <type_traits>

#include "analysis.h"
#include "computation.h"
#include "experiment.h"

#define CHECK_FOR_BINARY_OP(op_name, op_symbol)                                \
  struct has_no_##op_name {};                                                  \
  template <typename T, typename Arg>                                          \
  has_no_##op_name operator op_symbol(const T &, const Arg &);                 \
  template <typename T, typename Arg = T> struct has_##op_name {               \
    enum {                                                                     \
      value = !std::is_same<decltype(std::declval<T>()                         \
                                         op_symbol std::declval<Arg>()),       \
                            has_no_##op_name>::value                           \
    };                                                                         \
  };                                                                           \
  template <typename T, typename Arg = T>                                      \
  static constexpr bool has_##op_name##_v = has_##op_name<T, Arg>::value;

#define DEFINE_DELAYED_BINARY_OP(op_name, op_symbol)                           \
  template <                                                                   \
      typename Arg, typename V = U,                                            \
      std::enable_if_t<                                                        \
          ana::is_column_v<V> &&                                               \
              ana::is_column_v<typename Arg::action_type> &&                   \
              op_check::has_##op_name##_v<                                     \
                  cell_value_t<V>, cell_value_t<typename Arg::action_type>>,   \
          bool> = false>                                                       \
  auto operator op_symbol(Arg const &arg) const {                              \
    return this->m_analysis                                                    \
        ->define([](cell_value_t<V> const &me,                                 \
                    cell_value_t<typename Arg::action_type> const &you) {      \
          return me op_symbol you;                                             \
        })                                                                     \
        .evaluate(*this, arg);                                                 \
  }

#define CHECK_FOR_UNARY_OP(op_name, op_symbol)                                 \
  struct has_no_##op_name {};                                                  \
  template <typename T> has_no_##op_name operator op_symbol(const T &);        \
  template <typename T> struct has_##op_name {                                 \
    enum {                                                                     \
      value = !std::is_same<decltype(op_symbol std::declval<T>()),             \
                            has_no_##op_name>::value                           \
    };                                                                         \
  };                                                                           \
  template <typename T>                                                        \
  static constexpr bool has_##op_name##_v = has_##op_name<T>::value;

#define DEFINE_DELAYED_UNARY_OP(op_name, op_symbol)                            \
  template <typename V = U,                                                    \
            std::enable_if_t<ana::is_column_v<V> &&                            \
                                 op_check::has_##op_name##_v<cell_value_t<V>>, \
                             bool> = false>                                    \
  auto operator op_symbol() const {                                            \
    return this->m_analysis                                                    \
        ->define([](cell_value_t<V> const &me) { return (op_symbol me); })     \
        .evaluate(*this);                                                      \
  }

#define CHECK_FOR_SUBSCRIPT_OP()                                               \
  template <class T, class Index> struct has_subscript_impl {                  \
    template <class T1, class IndexDeduced = Index,                            \
              class Reference = decltype((                                     \
                  *std::declval<T *>())[std::declval<IndexDeduced>()]),        \
              typename = typename std::enable_if<                              \
                  !std::is_void<Reference>::value>::type>                      \
    static std::true_type test(int);                                           \
    template <class> static std::false_type test(...);                         \
    using type = decltype(test<T>(0));                                         \
  };                                                                           \
  template <class T, class Index>                                              \
  using has_subscript = typename has_subscript_impl<T, Index>::type;           \
  template <class T, class Index>                                              \
  static constexpr bool has_subscript_v = has_subscript<T, Index>::value;

#define DEFINE_SUBSCRIPT_OP()                                                  \
  template <typename Arg, typename V = U,                                      \
            std::enable_if_t<is_column_v<V> &&                                 \
                                 op_check::has_subscript_v<                    \
                                     cell_value_t<V>,                          \
                                     cell_value_t<typename Arg::action_type>>, \
                             bool> = false>                                    \
  auto operator[](Arg const &arg) const {                                      \
    return this->m_analysis->define(                                           \
        [](cell_value_t<V> me,                                                 \
           cell_value_t<typename Arg::action_type> index) {                    \
          return me[index];                                                    \
        })(*this, arg);                                                        \
  }

namespace ana {

namespace op_check {
CHECK_FOR_UNARY_OP(logical_not, !)
CHECK_FOR_UNARY_OP(minus, -)
CHECK_FOR_BINARY_OP(addition, +)
CHECK_FOR_BINARY_OP(subtraction, -)
CHECK_FOR_BINARY_OP(multiplication, *)
CHECK_FOR_BINARY_OP(division, /)
CHECK_FOR_BINARY_OP(remainder, %)
CHECK_FOR_BINARY_OP(greater_than, >)
CHECK_FOR_BINARY_OP(less_than, <)
CHECK_FOR_BINARY_OP(greater_than_or_equal_to, >=)
CHECK_FOR_BINARY_OP(less_than_or_equal_to, <=)
CHECK_FOR_BINARY_OP(equality, ==)
CHECK_FOR_BINARY_OP(inequality, !=)
CHECK_FOR_BINARY_OP(logical_and, &&)
CHECK_FOR_BINARY_OP(logical_or, ||)
CHECK_FOR_SUBSCRIPT_OP()
} // namespace op_check

template <typename Bkr> using booked_counter_t = typename Bkr::counter_type;

/**
 * @brief Node representing a lazy action to be performed in an analysis.
 * @details Depending on the concrete type of the lazy action, further
 * operations may be performed on it.
 * @tparam T Input dataset type
 * @tparam U Action to be performed lazily
 */
template <typename T>
template <typename U>
class analysis<T>::lazy : public node<U>, public concurrent<U> {

public:
  using analysis_type = typename node<U>::analysis_type;
  using dataset_type = typename node<U>::dataset_type;
  using action_type = typename node<U>::action_type;

  template <typename Sel, typename... Args>
  using lazy_selection_applicator_t =
      decltype(std::declval<analysis<T>>().template filter<Sel>(
          std::declval<std::string>(), std::declval<Args>()...));

  template <typename Sel, typename... Args>
  using selection_applicator_t =
      typename decltype(std::declval<analysis<T>>().template filter<Sel>(
          std::declval<std::string>(), std::declval<Args>()...))::action_type;

public:
  // friends with the main analysis graph & any other lazy nodes
  friend class analysis<T>;
  template <typename> friend class lazy;

public:
  lazy(analysis<T> &analysis, const concurrent<U> &action)
      : node<U>::node(analysis), concurrent<U>::concurrent(action) {}

  virtual ~lazy() = default;

  template <typename V>
  lazy(lazy<V> const &other)
      : node<U>::node(*other.m_analysis), concurrent<U>::concurrent(other) {}

  template <typename V> lazy &operator=(lazy<V> const &other) {
    concurrent<U>::operator=(other);
    this->m_analysis = other.m_analysis;
    return *this;
  }

  virtual void set_variation(const std::string &var_name,
                             const lazy &var) override;

  virtual lazy<U> get_nominal() const override;
  virtual lazy<U> get_variation(const std::string &var_name) const override;

  virtual bool has_variation(const std::string &var_name) const override;
  virtual std::set<std::string> list_variation_names() const override;

  /**
   * @brief Apply a systematic variation to a `reader` or `constant` column.
   * @param var_name Name of the systematic variation.
   * @param args... Alternate column name (`reader`) or value (`constant`).
   * @return Varied column.
   * @details Creates a `varied` action whose `.get_nominal()` is the original
   * lazy one, and `get_variation(var_name)` is the newly-constructed one.
   */
  template <typename... Args, typename V = U,
            std::enable_if_t<ana::column::template is_reader_v<V> ||
                                 ana::column::template is_constant_v<V>,
                             bool> = false>
  auto vary(const std::string &var_name, Args &&...args) -> varied<V>;

  /**
   * @brief Apply a systematic variation to an `equation` column.
   * @param var_name Name of the systematic variation.
   * @param args... Constructor arguments for `definition`.
   * @return Varied definition.
   * @details Creates a `varied` action whose `.get_nominal()` is the original
   * lazy one, and `get_variation(var_name)` is the newly-constructed one.
   */
  template <typename... Args, typename V = U,
            std::enable_if_t<ana::column::template is_evaluator_v<V> &&
                                 !ana::column::template is_equation_v<
                                     ana::column::template evaluated_t<V>>,
                             bool> = false>
  auto vary(const std::string &var_name, Args &&...args) -> varied<V>;

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
  template <typename F, typename V = U,
            std::enable_if_t<ana::column::template is_evaluator_v<V> &&
                                 ana::column::template is_equation_v<
                                     ana::column::template evaluated_t<V>>,
                             bool> = false>
  auto vary(const std::string &var_name, F callable) -> varied<V>;

  /**
   * @brief Filter from an existing selection.
   * @tparam Sel Type of selection to be applied, i.e. `ana::selection::cut` or
   * `ana::selection::weight`.
   * @tparam Args (Optional) Type of function/functor/callable expression.
   * @param name Name of the selection.
   * @param args (Optional) function/functor/callable expression to be applied.
   * @return Chained selection "applicator" to be applied with input columns.
   * @details Chained selections have their cut and weight decisions compounded:
   * ```cpp
   * auto sel = ds.channel<cut>("a")(a).filter<weight>("w")(w);
   * // cut = (a) && (true);
   * // weight = (1.0) * (w);
   * ```
   */
  template <typename Sel, typename... Args>
  auto filter(const std::string &name, Args &&...args)
      -> lazy_selection_applicator_t<Sel, Args...>;

  /**
   * @brief Channel from an existing selection.
   * @tparam Sel Type of selection to be applied, i.e. `ana::selection::cut` or
   * `ana::selection::weight`.
   * @param name Name of the selection.
   * @param args (Optional) lambda expression to be evaluated.
   * @return Chained selection (to be evaluated with input columns)
   * @details The name of the selection from which this method is called from
   * will be preserved as part of the path for chained selections:
   * ```cpp
   * auto sel = ds.channel<cut>("a")(a).filter<weight>("b")(b);
   * sel.get_path();  // "a/b"
   * ```
   */
  template <typename Sel, typename... Args>
  auto channel(const std::string &name, Args &&...args)
      -> lazy_selection_applicator_t<Sel, Args...>;

  /**
   * @brief Evaluate the column out of existing ones.
   * @param columns Input columns.
   * @return Evaluated column.
   * @details The input column(s) can be `lazy` or `varied`. Correspondingly,
   * the evaluated column will be as well.
   */
  template <
      typename... Nodes, typename V = U,
      std::enable_if_t<ana::column::template is_evaluator_v<V>, bool> = false>
  auto evaluate(Nodes &&...columns) const
      -> decltype(std::declval<lazy<V>>().evaluate_column(
          std::forward<Nodes>(columns)...)) {
    return this->evaluate_column(std::forward<Nodes>(columns)...);
  }

  template <typename... Nodes, typename V = U,
            std::enable_if_t<
                ana::column::template is_evaluator_v<V> &&
                    ana::analysis<T>::template has_no_variation_v<Nodes...>,
                bool> = false>
  auto evaluate_column(Nodes const &...columns) const
      -> lazy<column::template evaluated_t<V>> {
    // nominal
    return this->m_analysis->evaluate_column(*this, columns...);
  }

  template <
      typename... Nodes, typename V = U,
      std::enable_if_t<ana::column::template is_evaluator_v<V> &&
                           ana::analysis<T>::template has_variation_v<Nodes...>,
                       bool> = false>
  auto evaluate_column(Nodes const &...columns) const
      -> varied<column::template evaluated_t<V>> {
    // variations
    auto nom =
        this->m_analysis->evaluate_column(*this, columns.get_nominal()...);
    varied<column::template evaluated_t<V>> syst(nom);
    for (auto const &var_name : list_all_variation_names(columns...)) {
      auto var = this->m_analysis->evaluate_column(
          *this, columns.get_variation(var_name)...);
      syst.set_variation(var_name, var);
    }
    return syst;
  }

  /**
   * @brief Apply the selection's expression based on input columns.
   * @param columns Input columns.
   * @return `lazy/varied<selection>` Applied selection.
   */
  template <typename... Nodes, typename V = U,
            std::enable_if_t<ana::selection::template is_applicator_v<V>,
                             bool> = false>
  auto apply(Nodes &&...columns) const
      -> decltype(std::declval<lazy<V>>().apply_selection(
          std::forward<Nodes>(columns)...)) {
    return this->apply_selection(std::forward<Nodes>(columns)...);
  }

  template <typename... Nodes, typename V = U,
            std::enable_if_t<
                selection::template is_applicator_v<V> &&
                    ana::analysis<T>::template has_no_variation_v<Nodes...>,
                bool> = false>
  auto apply_selection(Nodes const &...columns) const -> lazy<selection> {
    // nominal
    return this->m_analysis->apply_selection(*this, columns...);
  }

  template <
      typename... Nodes, typename V = U,
      std::enable_if_t<selection::template is_applicator_v<V> &&
                           ana::analysis<T>::template has_variation_v<Nodes...>,
                       bool> = false>
  auto apply_selection(Nodes const &...columns) const -> varied<selection> {
    // variations
    varied<selection> syst(
        this->get_nominal().apply_selection(columns.get_nominal()...));
    auto var_names = list_all_variation_names(columns...);
    for (auto const &var_name : var_names) {
      syst.set_variation(var_name,
                         this->get_variation(var_name).apply_selection(
                             columns.get_variation(var_name)...));
    }
    return syst;
  }

  /**
   * @brief Fill the counter with input columns.
   * @param columns Input (`lazy` or `varied`) columns.
   * @return The counter (`lazy` or `varied`) filled with the input columns.
   */
  template <
      typename... Nodes, typename V = U,
      std::enable_if_t<ana::counter::template is_booker_v<V>, bool> = false>
  auto fill(Nodes &&...columns) const
      -> decltype(std::declval<lazy<V>>().fill_counter(
          std::declval<Nodes>()...));

  template <typename... Nodes, typename V = U,
            std::enable_if_t<
                ana::counter::template is_booker_v<V> &&
                    ana::analysis<T>::template has_no_variation_v<Nodes...>,
                bool> = false>
  auto fill_counter(Nodes const &...columns) const -> lazy<V> {
    // nominal
    return lazy<V>(*this->m_analysis,
                   this->get_concurrent_result(
                       [](V &fillable, typename Nodes::action_type &...cols) {
                         return fillable.book_fill(cols...);
                       },
                       columns...));
  }

  template <
      typename... Nodes, typename V = U,
      std::enable_if_t<ana::counter::template is_booker_v<V> &&
                           ana::analysis<T>::template has_variation_v<Nodes...>,
                       bool> = false>
  auto fill_counter(Nodes const &...columns) const -> varied<V> {
    // variations
    auto syst = varied<V>(this->fill_counter(columns.get_nominal()...));
    for (auto const &var_name : list_all_variation_names(columns...)) {
      syst.set_variation(
          var_name, this->fill_counter(columns.get_variation(var_name)...));
    }
    return syst;
  }

  /**
   * @brief Book the counter at a selection.
   * @param selection Selection to be counted.
   * @return The (`lazy` or `varied`) counter booked at the selection.
   */
  template <typename Node> auto at(Node &&selection) const;

  template <typename Node, typename V = U,
            std::enable_if_t<ana::counter::template is_booker_v<V> &&
                                 ana::analysis<T>::template is_nominal_v<Node>,
                             bool> = false>
  auto book_selection(Node const &sel) const -> lazy<booked_counter_t<V>> {
    // nominal
    return this->m_analysis->book_selection(*this, sel);
  }

  template <typename Node, typename V = U,
            std::enable_if_t<ana::counter::template is_booker_v<V> &&
                                 ana::analysis<T>::template is_varied_v<Node>,
                             bool> = false>
  auto book_selection(Node const &sel) const -> varied<booked_counter_t<V>> {
    // variations
    auto syst = varied<booked_counter_t<V>>(
        this->m_analysis->book_selection(*this, sel.get_nominal()));
    for (auto const &var_name : list_all_variation_names(sel)) {
      syst.set_variation(var_name, this->m_analysis->book_selection(
                                       *this, sel.get_variation(var_name)));
    }
    return syst;
  }

  /**
   * @brief Book the counter at multiple selections.
   * @param selections Selections to book the counter at.
   * @return The (`lazy` or `varied`) counter "booker" that keeps track the
   * counters at each selection.
   */
  template <typename... Nodes> auto at(Nodes &&...nodes) const {
    static_assert(counter::template is_booker_v<U>, "not a counter (booker)");
    return this->book_selections(std::forward<Nodes>(nodes)...);
  }

  template <typename... Nodes, typename V = U,
            std::enable_if_t<
                ana::counter::template is_booker_v<V> &&
                    ana::analysis<T>::template has_no_variation_v<Nodes...>,
                bool> = false>
  auto book_selections(Nodes const &...sels) const -> lazy<V> {
    // nominal
    return this->m_analysis->book_selections(*this, sels...);
  }

  template <
      typename... Nodes, typename V = U,
      std::enable_if_t<ana::counter::template is_booker_v<V> &&
                           ana::analysis<T>::template has_variation_v<Nodes...>,
                       bool> = false>
  auto book_selections(Nodes const &...sels) const -> varied<V> {
    // variations
    auto syst = varied<V>(
        this->m_analysis->book_selections(*this, sels.get_nominal()...));
    for (auto const &var_name : list_all_variation_names(sels...)) {
      syst.set_variation(var_name, this->m_analysis->book_selections(
                                       *this, sels.get_variation(var_name)...));
    }
    return syst;
  }

  template <typename V = U,
            std::enable_if_t<ana::is_selection_v<V>, bool> = false>
  std::string get_path() const {
    return this->get_model_value(
        [](const selection &me) { return me.get_path(); });
  }

  /**
   * @return The list of booked selection paths.
   */
  template <
      typename V = U,
      std::enable_if_t<ana::counter::template is_booker_v<V>, bool> = false>
  auto list_selection_paths() const -> std::set<std::string> {
    return this->get_model_value(
        [](U const &bkr) { return bkr.list_selection_paths(); });
  }

  /**
   * @brief Get the counter booked at a selection path.
   * @param selection_path Path of the selection
   * @return counter The counter booked at the selection path.
   */
  template <
      typename V = U,
      std::enable_if_t<ana::counter::template is_booker_v<V>, bool> = false>
  auto get_counter(const std::string &sel_path) const
      -> lazy<booked_counter_t<V>> {
    return lazy<typename V::counter_type>(
        *this->m_analysis,
        this->get_concurrent_result([sel_path = sel_path](U &bkr) {
          return bkr.get_counter(sel_path);
        }));
  }

  /**
   * @brief Retrieve the result of a counter.
   * @details Triggers processing of the dataset if that the result is not
   * already available.
   * @return The result of the implemented counter.
   */
  template <typename V = U,
            std::enable_if_t<ana::counter::template is_implemented_v<V>, bool> =
                false>
  auto get_result() const -> decltype(std::declval<V>().get_result()) {
    this->m_analysis->analyze();
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
  template <typename... Args, typename V = U,
            std::enable_if_t<column::template is_evaluator_v<V> ||
                                 selection::template is_applicator_v<V>,
                             bool> = false>
  auto operator()(Args &&...columns)
      -> decltype(std::declval<lazy<V>>().evaluate_or_apply(
          std::forward<Args>(std::declval<Args &&>())...)) {
    return this->evaluate_or_apply(std::forward<Args>(columns)...);
  }

  template <typename... Args, typename V = U,
            std::enable_if_t<column::template is_evaluator_v<V>, bool> = false>
  auto evaluate_or_apply(Args &&...columns)
      -> decltype(std::declval<lazy<V>>().evaluate(
          std::forward<Args>(std::declval<Args &&>())...)) {
    return this->evaluate(std::forward<Args>(columns)...);
  }

  template <
      typename... Args, typename V = U,
      std::enable_if_t<selection::template is_applicator_v<V>, bool> = false>
  auto evaluate_or_apply(Args &&...columns)
      -> decltype(std::declval<lazy<V>>().apply(
          std::forward<Args>(std::declval<Args &&>())...)) {
    return this->apply(std::forward<Args>(columns)...);
  }

  /**
   * @brief Take the OR of two cuts
   * @return selection Its cut decision is given by `passed_cut() =
   * a.passed_cut()
   * || b.passed_cut()`.
   * @details A joined filter is defined as a cut without any
   * preselection (i.e. weight = 1.0), and one that is not a `channel`.
   */
  template <typename V = U,
            std::enable_if_t<ana::is_selection_v<V>, bool> = false>
  auto operator||(const lazy<selection> &b) const -> lazy<selection> {
    return this->m_analysis->template join<selection::cut::a_or_b>(*this, b);
  }

  /**
   * @brief Take the AND of two cuts
   * @return `lazy<selection>` Its decision is given by `passed_cut() =
   * a.passed_cut() && b.passed_cut()`.
   * @details A joined filter is defined as a cut without any
   * preselection (i.e. weight = 1.0), and one that is not a `channel`.
   */
  template <typename V = U,
            std::enable_if_t<ana::is_selection_v<V>, bool> = false>
  auto operator&&(const lazy<selection> &b) const -> lazy<selection> {
    return this->m_analysis->template join<selection::cut::a_and_b>(*this, b);
  }

  /**
   * @brief Join two filters (OR)
   * @return selection Its decision is given by `passed_cut() = a.passed_cut()
   * || b.passed_cut()`.
   * @details A joined filter is a cut without any preselection (i.e. weight
   * = 1.0), and one that cannot be designated as a `channel`.
   */
  template <typename V = U,
            std::enable_if_t<ana::is_selection_v<V>, bool> = false>
  auto operator||(const varied<selection> &b) const -> lazy<selection> {
    varied<selection> syst(this->get_nominal().operator||(b.get_nominal()));
    auto var_names = list_all_variation_names(b);
    for (auto const &var_name : var_names) {
      syst.set_variation(var_name, this->get_variation(var_name).operator||(
                                       b.get_variation(var_name)));
    }
    return syst;
  }

  /**
   * @brief Join two filters (AND)
   * @return `lazy<selection>` Its decision is given by `passed_cut() =
   * a.passed_cut() && b.passed_cut()`.
   * @details A joined filter is a cut without any preselection (i.e. weight
   * = 1.0), and one that cannot be designated as a `channel`.
   */
  template <typename V = U,
            std::enable_if_t<ana::is_selection_v<V>, bool> = false>
  auto operator&&(const varied<selection> &b) const -> lazy<selection> {
    varied<selection> syst(this->get_nominal().operator&&(b.get_nominal()));
    auto var_names = list_all_variation_names(b);
    for (auto const &var_name : var_names) {
      syst.set_variation(var_name, this->get_variation(var_name).operator&&(
                                       b.get_variation(var_name)));
    }
    return syst;
  }

  /**
   * @brief Shorthand for `get_counter` of counter booker.
   * @param sel_path The path of booked selection.
   * @return Counter the `lazy` counter booked at the selection.
   */
  template <
      typename V = U,
      std::enable_if_t<ana::counter::template is_booker_v<V>, bool> = false>
  auto operator[](const std::string &sel_path) const
      -> lazy<booked_counter_t<V>>
  // subscript = access a counter at a selection path
  {
    return this->get_counter(sel_path);
  }

  /**
   * @brief Shorthand for `result` of counter.
   * @return `Result` the result of the implemented counter.
   */
  template <typename V = U,
            std::enable_if_t<ana::counter::template is_implemented_v<V>, bool> =
                false>
  auto operator->() const -> decltype(std::declval<V>().get_result()) {
    return this->get_result();
  }

  DEFINE_SUBSCRIPT_OP()
  DEFINE_DELAYED_UNARY_OP(logical_not, !)
  DEFINE_DELAYED_UNARY_OP(minus, -)
  DEFINE_DELAYED_BINARY_OP(equality, ==)
  DEFINE_DELAYED_BINARY_OP(inequality, !=)
  DEFINE_DELAYED_BINARY_OP(addition, +)
  DEFINE_DELAYED_BINARY_OP(subtraction, -)
  DEFINE_DELAYED_BINARY_OP(multiplication, *)
  DEFINE_DELAYED_BINARY_OP(division, /)
  DEFINE_DELAYED_BINARY_OP(logical_or, ||)
  DEFINE_DELAYED_BINARY_OP(logical_and, &&)
  DEFINE_DELAYED_BINARY_OP(greater_than, >)
  DEFINE_DELAYED_BINARY_OP(less_than, <)
  DEFINE_DELAYED_BINARY_OP(greater_than_or_equal_to, >=)
  DEFINE_DELAYED_BINARY_OP(less_than_or_equal_to, <=)

protected:
  template <typename V = U,
            typename std::enable_if<ana::counter::template is_implemented_v<V>,
                                    void>::type * = nullptr>
  void merge_results() const {
    auto model = this->get_model();
    if (!model->is_merged()) {
      std::vector<std::decay_t<decltype(model->get_result())>> results;
      for (size_t islot = 0; islot < this->concurrency(); ++islot) {
        auto slot = this->get_slot(islot);
        results.push_back(slot->get_result());
      }
      model->merge_results(results);
    }
  }
};

template <typename T> using analysis_t = typename T::analysis_type;
template <typename T> using action_t = typename T::action_type;

} // namespace ana

#include "definition.h"
#include "equation.h"
#include "reader.h"
#include "varied.h"

template <typename T>
template <typename Act>
void ana::analysis<T>::lazy<Act>::set_variation(const std::string &,
                                                lazy const &) {
  // should never be called
  throw std::logic_error("cannot set variation to a lazy action");
}

template <typename T>
template <typename Act>
auto ana::analysis<T>::lazy<Act>::get_nominal() const -> lazy<Act> {
  // this is nominal
  return *this;
}

template <typename T>
template <typename Act>
auto ana::analysis<T>::lazy<Act>::get_variation(const std::string &) const
    -> lazy<Act> {
  // propagation of variations must occur "transparently"
  return *this;
}

template <typename T>
template <typename Act>
std::set<std::string>
ana::analysis<T>::lazy<Act>::list_variation_names() const {
  // no variations to list
  return std::set<std::string>();
}

template <typename T>
template <typename Act>
bool ana::analysis<T>::lazy<Act>::has_variation(const std::string &) const {
  // always false
  return false;
}

template <typename T>
template <typename Act>
template <typename... Args, typename V,
          std::enable_if_t<ana::column::template is_reader_v<V> ||
                               ana::column::template is_constant_v<V>,
                           bool>>
auto ana::analysis<T>::lazy<Act>::vary(const std::string &var_name,
                                       Args &&...args) -> varied<V> {
  // create a lazy varied with the this as nominal
  auto syst = varied<V>(*this);
  // set variation of the column according to new constructor arguments
  syst.set_variation(var_name, this->m_analysis->vary_column(
                                   *this, std::forward<Args>(args)...));
  // done
  return syst;
}

template <typename T>
template <typename Act>
template <typename... Args, typename V,
          std::enable_if_t<ana::column::template is_evaluator_v<V> &&
                               !ana::column::template is_equation_v<
                                   ana::column::template evaluated_t<V>>,
                           bool>>
auto ana::analysis<T>::lazy<Act>::vary(const std::string &var_name,
                                       Args &&...args) -> varied<V> {
  // create a lazy varied with the this as nominal
  auto syst = varied<V>(*this);
  // set variation of the column according to new constructor arguments
  syst.set_variation(var_name, this->m_analysis->vary_definition(
                                   *this, std::forward<Args>(args)...));
  // done
  return syst;
}

template <typename T>
template <typename Act>
template <typename F, typename V,
          std::enable_if_t<ana::column::template is_evaluator_v<V> &&
                               ana::column::template is_equation_v<
                                   ana::column::template evaluated_t<V>>,
                           bool>>
auto ana::analysis<T>::lazy<Act>::vary(const std::string &var_name, F callable)
    -> varied<V> {
  // create a lazy varied with the this as nominal
  auto syst = varied<V>(*this);
  // set variation of the column according to new constructor arguments
  syst.set_variation(var_name,
                     this->m_analysis->vary_equation(*this, callable));
  // done
  return syst;
}

template <typename T>
template <typename Act>
template <typename Sel, typename... Args>
auto ana::analysis<T>::lazy<Act>::filter(const std::string &name,
                                         Args &&...args)
    -> lazy_selection_applicator_t<Sel, Args...> {
  if constexpr (std::is_base_of_v<selection, Act>) {
    return this->m_analysis->template filter<Sel>(*this, name,
                                                  std::forward<Args>(args)...);
  } else {
    static_assert(std::is_base_of_v<selection, Act>,
                  "filter must be called from a selection");
  }
}

template <typename T>
template <typename Act>
template <typename Sel, typename... Args>
auto ana::analysis<T>::lazy<Act>::channel(const std::string &name,
                                          Args &&...args)
    -> lazy_selection_applicator_t<Sel, Args...> {
  if constexpr (std::is_base_of_v<selection, Act>) {
    return this->m_analysis->template channel<Sel>(*this, name,
                                                   std::forward<Args>(args)...);
  } else {
    static_assert(std::is_base_of_v<selection, Act>,
                  "channel must be called from a selection");
  }
}

template <typename T>
template <typename Act>
template <typename... Nodes, typename V,
          std::enable_if_t<ana::counter::template is_booker_v<V>, bool>>
auto ana::analysis<T>::lazy<Act>::fill(Nodes &&...columns) const
    -> decltype(std::declval<lazy<V>>().fill_counter(
        std::declval<Nodes>()...)) {
  static_assert(counter::template is_booker_v<V>,
                "non-counter(booker) cannot be filled");
  return this->fill_counter(std::forward<Nodes>(columns)...);
}

template <typename T>
template <typename Act>
template <typename Node>
auto ana::analysis<T>::lazy<Act>::at(Node &&selection) const {
  static_assert(counter::template is_booker_v<Act>, "not a counter (booker)");
  return this->book_selection(std::forward<Node>(selection));
}