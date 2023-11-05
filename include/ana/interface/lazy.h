#pragma once

/** @file */

#include <iostream>
#include <set>
#include <type_traits>

#include "dataflow.h"
#include "systematic_resolver.h"

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
      typename Arg, typename V = Action,                                       \
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
  template <typename V = Action,                                               \
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
      typename Arg, typename V = Action,                                       \
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

template <typename Action>
class lazy : public systematic::resolver<lazy<Action>>,
             public lockstep::view<Action> {

public:
  class varied;

public:
  using operation_type = Action;

public:
  friend class dataflow;
  template <typename> friend class lazy;

public:
  lazy(dataflow &dataflow, const lockstep::view<Action> &operation)
      : systematic::resolver<lazy<Action>>::resolver(dataflow),
        lockstep::view<Action>::view(operation) {}
  lazy(dataflow &dataflow, const lockstep::node<Action> &operation)
      : systematic::resolver<lazy<Action>>::resolver(dataflow),
        lockstep::view<Action>::view(operation) {}

  lazy(const lazy &) = default;
  lazy &operator=(const lazy &) = default;

  template <typename Derived> lazy(lazy<Derived> const &derived);
  template <typename Derived> lazy &operator=(lazy<Derived> const &derived);

  virtual ~lazy() = default;

  virtual void set_variation(const std::string &var_name, lazy &&var) override;

  virtual lazy const &nominal() const override;
  virtual lazy const &variation(const std::string &var_name) const override;

  virtual bool has_variation(const std::string &var_name) const override;
  virtual std::set<std::string> list_variation_names() const override;

  template <typename... Args>
  auto filter(const std::string &name, Args &&...args) const;

  template <typename... Args>
  auto weight(const std::string &name, Args &&...args) const;

  template <typename... Args>
  auto channel(const std::string &name, Args &&...args) const;

  template <typename Agg> auto book(Agg &&agg) const;
  template <typename... Aggs> auto book(Aggs &&...aggs) const;

  template <typename V = Action,
            std::enable_if_t<is_selection_v<V>, bool> = false>
  std::string path() const {
    return this->get_model_value(
        [](const selection &me) { return me.get_path(); });
  }

  template <typename V = Action,
            std::enable_if_t<ana::aggregation::template has_output_v<V>, bool> =
                false>
  auto result() const;
  /**
   * @brief Shorthand for `result` of aggregation.
   * @return `Result` the result of the implemented aggregation.
   */
  template <typename V = Action,
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
  template <typename V = Action,
            std::enable_if_t<ana::aggregation::template has_output_v<V>, bool> =
                false>
  void merge_results() const;
};

} // namespace ana

#include "column.h"
#include "delayed.h"
#include "lazy_varied.h"

template <typename Action>
template <typename Derived>
ana::lazy<Action>::lazy(lazy<Derived> const &derived)
    : ana::systematic::resolver<lazy<Action>>(*derived.m_df),
      ana::lockstep::view<Action>(derived) {
  this->m_df = derived.m_df;
}

template <typename Action>
template <typename Derived>
ana::lazy<Action> &ana::lazy<Action>::operator=(lazy<Derived> const &derived) {
  lockstep::view<Action>::operator=(derived);
  this->m_df = derived.m_df;
  return *this;
}

template <typename Action>
void ana::lazy<Action>::set_variation(const std::string &, lazy &&) {
  // should never be called
  throw std::logic_error("cannot set variation to a lazy operation");
}

template <typename Action>
auto ana::lazy<Action>::nominal() const -> lazy const & {
  // this is nominal
  return *this;
}

template <typename Action>
auto ana::lazy<Action>::variation(const std::string &) const -> lazy const & {
  // propagation of variations must occur "transparently"
  return *this;
}

template <typename Action>
std::set<std::string> ana::lazy<Action>::list_variation_names() const {
  // no variations to list
  return std::set<std::string>();
}

template <typename Action>
bool ana::lazy<Action>::has_variation(const std::string &) const {
  // always false
  return false;
}

template <typename Action>
template <typename... Args>
auto ana::lazy<Action>::filter(const std::string &name, Args &&...args) const {
  if constexpr (std::is_base_of_v<selection, Action>) {
    return this->m_df->template select<selection::cut>(
        *this, name, std::forward<Args>(args)...);
  } else {
    static_assert(std::is_base_of_v<selection, Action>,
                  "filter must be called from a selection");
  }
}

template <typename Action>
template <typename... Args>
auto ana::lazy<Action>::weight(const std::string &name, Args &&...args) const {
  if constexpr (std::is_base_of_v<selection, Action>) {
    return this->m_df->template select<selection::weight>(
        *this, name, std::forward<Args>(args)...);
  } else {
    static_assert(std::is_base_of_v<selection, Action>,
                  "weight must be called from a selection");
  }
}

template <typename Action>
template <typename... Args>
auto ana::lazy<Action>::channel(const std::string &name, Args &&...args) const {
  if constexpr (std::is_base_of_v<selection, Action>) {
    return this->m_df->template channel<selection::weight>(
        *this, name, std::forward<Args>(args)...);
  } else {
    static_assert(std::is_base_of_v<selection, Action>,
                  "channel must be called from a selection");
  }
}

template <typename Action>
template <typename Agg>
auto ana::lazy<Action>::book(Agg &&agg) const {
  static_assert(std::is_base_of_v<selection, Action>,
                "book must be called from a selection");
  return agg.book(*this);
}

template <typename Action>
template <typename... Aggs>
auto ana::lazy<Action>::book(Aggs &&...aggs) const {
  static_assert(std::is_base_of_v<selection, Action>,
                "book must be called from a selection");
  return std::make_tuple((aggs.book(*this), ...));
}

template <typename Action>
template <typename V,
          std::enable_if_t<ana::aggregation::template has_output_v<V>, bool>>
auto ana::lazy<Action>::result() const {
  this->m_df->analyze();
  this->merge_results();
  return this->get_model()->get_result();
}

template <typename Action>
template <typename V,
          std::enable_if_t<ana::aggregation::template has_output_v<V>, bool> e>
void ana::lazy<Action>::merge_results() const {
  auto model = this->get_model();
  if (!model->is_merged()) {
    std::vector<std::decay_t<decltype(model->get_result())>> results;
    results.reserve(this->concurrency());
    for (size_t islot = 0; islot < this->concurrency(); ++islot) {
      results.push_back(this->get_slot(islot)->get_result());
    }
    model->merge_results(results);
  }
}