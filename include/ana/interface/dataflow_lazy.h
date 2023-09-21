#pragma once

/** @file */

#include <iostream>
#include <set>
#include <type_traits>

#include "dataflow.h"
#include "dataflow_systematic.h"

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

#define DEFINE_LAZY_BINARY_OP(op_name, op_symbol)                              \
  template <                                                                   \
      typename Arg, typename V = U,                                            \
      std::enable_if_t<ana::is_column_v<V> &&                                  \
                           ana::is_column_v<typename Arg::operation_type> &&   \
                           op_check::has_##op_name##_v<                        \
                               cell_value_t<V>,                                \
                               cell_value_t<typename Arg::operation_type>>,    \
                       bool> = false>                                          \
  auto operator op_symbol(Arg const &arg) const {                              \
    return this->m_df                                                          \
        ->define([](cell_value_t<V> const &me,                                 \
                    cell_value_t<typename Arg::operation_type> const &you) {   \
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

#define DEFINE_LAZY_UNARY_OP(op_name, op_symbol)                               \
  template <typename V = U,                                                    \
            std::enable_if_t<ana::is_column_v<V> &&                            \
                                 op_check::has_##op_name##_v<cell_value_t<V>>, \
                             bool> = false>                                    \
  auto operator op_symbol() const {                                            \
    return this->m_df                                                          \
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

#define DEFINE_LAZY_SUBSCRIPT_OP()                                             \
  template <                                                                   \
      typename Arg, typename V = U,                                            \
      std::enable_if_t<is_column_v<V> &&                                       \
                           op_check::has_subscript_v<                          \
                               cell_value_t<V>,                                \
                               cell_value_t<typename Arg::operation_type>>,    \
                       bool> = false>                                          \
  auto operator[](Arg const &arg) const {                                      \
    return this->m_df->define(                                                 \
        [](cell_value_t<V> me,                                                 \
           cell_value_t<typename Arg::operation_type> index) {                 \
          return me[index];                                                    \
        })(*this, arg);                                                        \
  }

namespace ana {

namespace op_check {
CHECK_FOR_UNARY_OP(logical_not, !)
CHECK_FOR_UNARY_OP(minus, -)
CHECK_FOR_BINARY_OP(addition, +)
CHECK_FOR_BINARY_OP(subtroperation, -)
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

/**
 * @brief Node representing a operation to be performed in an analysis.
 * @details Lazy nodes represent the final operation to be performed in a
 * dataset, pending its processing. It can be provided to other dataflow
 * operations as inputs.
 * @tparam T Input dataset type
 * @tparam U Action to be performed lazily
 */
template <typename T>
template <typename U>
class dataflow<T>::lazy : public systematic<lazy<U>>, public lockstep::view<U> {

public:
  class varied;

public:
  using dataflow_type = typename systematic<lazy<U>>::dataflow_type;
  using dataset_type = typename systematic<lazy<U>>::dataset_type;
  using operation_type = U;

  template <typename Sel, typename... Args>
  using delayed_selection_applicator_t =
      decltype(std::declval<dataflow<T>>().template filter<Sel>(
          std::declval<std::string>(), std::declval<Args>()...));

public:
  // friends with the main dataflow graph & any other lazy nodes
  friend class dataflow<T>;
  template <typename> friend class lazy;

public:
  lazy(dataflow<T> &dataflow, const lockstep::view<U> &operation)
      : systematic<lazy<U>>::systematic(dataflow), lockstep::view<U>::view(
                                                       operation) {}
  lazy(dataflow<T> &dataflow, const lockstep::node<U> &operation)
      : systematic<lazy<U>>::systematic(dataflow), lockstep::view<U>::view(
                                                       operation) {}

  lazy(const lazy &) = default;
  lazy &operator=(const lazy &) = default;

  virtual ~lazy() = default;

  virtual void set_variation(const std::string &var_name, lazy &&var) override;

  virtual lazy const &nominal() const override;
  virtual lazy const &variation(const std::string &var_name) const override;

  virtual bool has_variation(const std::string &var_name) const override;
  virtual std::set<std::string> list_variation_names() const override;

  /**
   * @brief Apply a systematic variation to a `reader` or `constant` column.
   * @param var_name Name of the systematic variation.
   * @param args... Alternate column name (`reader`) or value (`constant`).
   * @return Varied column.
   * @details Creates a `varied` operation whose `.nominal()` is the original
   * lazy one, and `variation(var_name)` is the newly-constructed one.
   */
  template <typename... Args, typename V = U,
            std::enable_if_t<ana::column::template is_reader_v<V> ||
                                 ana::column::template is_constant_v<V>,
                             bool> = false>
  auto vary(const std::string &var_name, Args &&...args) -> varied;

  /**
   * @brief Filter from an existing selection.
   * @tparam Sel Type of selection to be applied, i.e. `ana::selection::cut` or
   * `ana::selection::weight`.
   * @tparam Args (Optional) Type of function/functor/callable expression.
   * @param name Name of the selection.
   * @param args (Optional) function/functor/callable expression to be used.
   * @return Selection to be applied with input columns.
   * @details Chained selections have their cut and weight decisions compounded:
   * ```cpp
   * auto sel =
   * ds.channel<cut>("a")(a).filter<weight>("b")(b).filter<cut>("c")(c);
   * // cut = (a) && (true) && (c);
   * // weight = (1.0) * (w) * (1.0);
   * ```
   */
  template <typename Sel, typename... Args>
  auto filter(const std::string &name, Args &&...args) const
      -> delayed_selection_applicator_t<Sel, Args...>;

  /**
   * @brief Channel from an existing selection.
   * @tparam Sel Type of selection to be applied, i.e. `ana::selection::cut` or
   * `ana::selection::weight`.
   * @param name Name of the selection.
   * @param args (Optional) function/functor/callable expression to be used.
   * @return Selection to be applied with input columns.
   * @details The name of the selection from which this method is called from
   * will be preserved as part of the path for chained selections:
   * ```cpp
   * auto sel =
   * ds.channel<cut>("a")(a).filter<weight>("b")(b).filter<cut>("c")(c);
   * sel.path();  // "a/c"
   * ```
   */
  template <typename Sel, typename... Args>
  auto channel(const std::string &name, Args &&...args) const
      -> delayed_selection_applicator_t<Sel, Args...>;

  template <typename V = U,
            std::enable_if_t<ana::is_selection_v<V>, bool> = false>
  std::string path() const {
    return this->get_model_value(
        [](const selection &me) { return me.get_path(); });
  }

  /**
   * @brief Retrieve the result of a aggregation.
   * @details Triggers processing of the dataset if that the result is not
   * already available.
   * @return The result of the implemented aggregation.
   */
  template <typename V = U,
            std::enable_if_t<ana::aggregation::template has_output_v<V>, bool> =
                false>
  auto result() const -> decltype(std::declval<V>().get_result()) {
    this->m_df->analyze();
    this->merge_results();
    return this->get_model()->get_result();
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
    return this->m_df->template join<selection::cut::a_or_b>(*this, b);
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
    return this->m_df->template join<selection::cut::a_and_b>(*this, b);
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
  auto operator||(typename lazy<selection>::varied const &b) const ->
      typename lazy<selection>::varied {
    using varied_type = typename lazy<selection>::varied;
    auto syst = varied_type(this->nominal().operator||(b.nominal()));
    auto var_names = list_all_variation_names(b);
    for (auto const &var_name : var_names) {
      syst.set_variation(var_name, this->variation(var_name).operator||(
                                       b.variation(var_name)));
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
  auto operator&&(typename lazy<selection>::varied const &b) const ->
      typename lazy<selection>::varied {
    using varied_type = typename lazy<selection>::varied;
    auto syst = varied_type(this->nominal().operator||(b.nominal()));
    auto var_names = list_all_variation_names(b);
    for (auto const &var_name : var_names) {
      syst.set_variation(var_name, this->variation(var_name).operator&&(
                                       b.variation(var_name)));
    }
    return syst;
  }

  /**
   * @brief Shorthand for `result` of aggregation.
   * @return `Result` the result of the implemented aggregation.
   */
  template <typename V = U,
            std::enable_if_t<ana::aggregation::template has_output_v<V>, bool> =
                false>
  auto operator->() const -> decltype(std::declval<V>().get_result()) {
    return this->result();
  }

  DEFINE_LAZY_SUBSCRIPT_OP()
  DEFINE_LAZY_UNARY_OP(logical_not, !)
  DEFINE_LAZY_UNARY_OP(minus, -)
  DEFINE_LAZY_BINARY_OP(equality, ==)
  DEFINE_LAZY_BINARY_OP(inequality, !=)
  DEFINE_LAZY_BINARY_OP(addition, +)
  DEFINE_LAZY_BINARY_OP(subtroperation, -)
  DEFINE_LAZY_BINARY_OP(multiplication, *)
  DEFINE_LAZY_BINARY_OP(division, /)
  DEFINE_LAZY_BINARY_OP(logical_or, ||)
  DEFINE_LAZY_BINARY_OP(logical_and, &&)
  DEFINE_LAZY_BINARY_OP(greater_than, >)
  DEFINE_LAZY_BINARY_OP(less_than, <)
  DEFINE_LAZY_BINARY_OP(greater_than_or_equal_to, >=)
  DEFINE_LAZY_BINARY_OP(less_than_or_equal_to, <=)

protected:
  template <typename V = U,
            std::enable_if_t<ana::aggregation::template has_output_v<V>, bool> =
                false>
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

} // namespace ana

#include "column.h"
#include "dataflow_lazy_varied.h"

template <typename T>
template <typename Act>
void ana::dataflow<T>::lazy<Act>::set_variation(const std::string &, lazy &&) {
  // should never be called
  throw std::logic_error("cannot set variation to a lazy operation");
}

template <typename T>
template <typename Act>
auto ana::dataflow<T>::lazy<Act>::nominal() const -> lazy const & {
  // this is nominal
  return *this;
}

template <typename T>
template <typename Act>
auto ana::dataflow<T>::lazy<Act>::variation(const std::string &) const
    -> lazy const & {
  // propagation of variations must occur "transparently"
  return *this;
}

template <typename T>
template <typename Act>
std::set<std::string>
ana::dataflow<T>::lazy<Act>::list_variation_names() const {
  // no variations to list
  return std::set<std::string>();
}

template <typename T>
template <typename Act>
bool ana::dataflow<T>::lazy<Act>::has_variation(const std::string &) const {
  // always false
  return false;
}

template <typename T>
template <typename Act>
template <typename Sel, typename... Args>
auto ana::dataflow<T>::lazy<Act>::filter(const std::string &name,
                                         Args &&...args) const
    -> delayed_selection_applicator_t<Sel, Args...> {
  if constexpr (std::is_base_of_v<selection, Act>) {
    return this->m_df->template filter<Sel>(*this, name,
                                            std::forward<Args>(args)...);
  } else {
    static_assert(std::is_base_of_v<selection, Act>,
                  "filter must be called from a selection");
  }
}

template <typename T>
template <typename Act>
template <typename Sel, typename... Args>
auto ana::dataflow<T>::lazy<Act>::channel(const std::string &name,
                                          Args &&...args) const
    -> delayed_selection_applicator_t<Sel, Args...> {
  if constexpr (std::is_base_of_v<selection, Act>) {
    return this->m_df->template channel<Sel>(*this, name,
                                             std::forward<Args>(args)...);
  } else {
    static_assert(std::is_base_of_v<selection, Act>,
                  "channel must be called from a selection");
  }
}

template <typename T>
template <typename Act>
template <typename... Args, typename V,
          std::enable_if_t<ana::column::template is_reader_v<V> ||
                               ana::column::template is_constant_v<V>,
                           bool>>
auto ana::dataflow<T>::lazy<Act>::vary(const std::string &var_name,
                                       Args &&...args) -> varied {
  // create a lazy varied with the this as nominal
  auto syst = varied(std::move(*this));
  // set variation of the column according to new constructor arguments
  syst.set_variation(
      var_name,
      this->m_df->vary_column(syst.nominal(), std::forward<Args>(args)...));
  // done
  return syst;
}