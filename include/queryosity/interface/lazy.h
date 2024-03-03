#pragma once

/** @file */

#include <iostream>
#include <memory>
#include <set>
#include <type_traits>
#include <vector>

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
  template <typename Arg, typename V = Action,                                 \
            std::enable_if_t<                                                  \
                queryosity::is_column_v<V> &&                                  \
                    queryosity::is_column_v<typename Arg::action_type> &&      \
                    op_check::has_##op_name##_v<                               \
                        column::template value_t<V>,                           \
                        column::template value_t<typename Arg::action_type>>,  \
                bool> = false>                                                 \
  auto operator op_symbol(Arg const &arg) const {                              \
    return this->m_df->define(                                                 \
        queryosity::column::expression(                                        \
            [](column::template value_t<V> const &me,                          \
               column::template value_t<typename Arg::action_type> const       \
                   &you) { return me op_symbol you; }),                        \
        *this, arg);                                                           \
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
            std::enable_if_t<                                                  \
                queryosity::is_column_v<V> &&                                  \
                    op_check::has_##op_name##_v<column::template value_t<V>>,  \
                bool> = false>                                                 \
  auto operator op_symbol() const {                                            \
    return this->m_df->define(queryosity::column::expression(                  \
                                  [](column::template value_t<V> const &me) {  \
                                    return (op_symbol me);                     \
                                  }),                                          \
                              *this);                                          \
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
  template <typename Arg, typename V = Action,                                 \
            std::enable_if_t<                                                  \
                is_column_v<V> &&                                              \
                    op_check::has_subscript_v<                                 \
                        column::template value_t<V>,                           \
                        column::template value_t<typename Arg::action_type>>,  \
                bool> = false>                                                 \
  auto operator[](Arg const &arg) const {                                      \
    return this->m_df->define(                                                 \
        queryosity::column::expression(                                        \
            [](column::template value_t<V> me,                                 \
               column::template value_t<typename Arg::action_type> index) {    \
              return me[index];                                                \
            }),                                                                \
        *this, arg);                                                           \
  }

namespace queryosity {

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

// mixin class to conditionally add a member variable
template <typename Action, typename Enable = void>
struct result_if_aggregation {};

// Specialization for types satisfying is_query
template <typename Action>
struct result_if_aggregation<
    Action, std::enable_if_t<query::template is_aggregation_v<Action>>> {
  using result_type = decltype(std::declval<Action>().result());

protected:
  result_type m_result;
};

template <typename Action>
class lazy : public dataflow::node,
             public concurrent::slotted<Action>,
             public systematic::resolver<lazy<Action>> {
  //  public result_if_aggregation<Action> {

public:
  class varied;

public:
  using action_type = Action;

public:
  friend class dataflow;
  template <typename> friend class lazy;

public:
  lazy(dataflow &df, std::vector<Action *> const &slots)
      : dataflow::node(df), m_slots(slots) {}
  lazy(dataflow &df, std::vector<std::unique_ptr<Action>> const &slots)
      : dataflow::node(df) {
    this->m_slots.reserve(slots.size());
    for (auto const &slot : slots) {
      this->m_slots.push_back(slot.get());
    }
  }

  lazy(const lazy &) = delete;
  lazy &operator=(const lazy &) = delete;

  lazy(lazy &&) = default;
  lazy &operator=(lazy &&) = default;

  template <typename Derived> lazy(lazy<Derived> const &derived);
  template <typename Derived> lazy &operator=(lazy<Derived> const &derived);

  virtual ~lazy() = default;

  virtual Action *get_slot(unsigned int islot) const override;
  virtual unsigned int concurrency() const override;

  virtual void set_variation(const std::string &var_name, lazy var) override;

  virtual lazy &nominal() override;
  virtual lazy &variation(const std::string &var_name) override;
  virtual lazy const &nominal() const override;
  virtual lazy const &variation(const std::string &var_name) const override;

  virtual bool has_variation(const std::string &var_name) const override;
  virtual std::set<std::string> list_variation_names() const override;

  template <typename Col> auto filter(lazy<Col> const &col) const;
  template <typename Col> auto weight(lazy<Col> const &col) const;

  template <typename Col> auto filter(Col const &col) const;
  template <typename Col> auto weight(Col const &col) const;

  template <typename Expr, typename... Cols>
  auto filter(queryosity::column::expression<Expr> const &expr,
              Cols const &...cols) const;

  template <typename Expr, typename... Cols>
  auto weight(queryosity::column::expression<Expr> const &expr,
              Cols const &...cols) const;

  template <typename Agg> auto book(Agg &&agg) const;
  template <typename... Aggs> auto book(Aggs &&...aggs) const;

  template <typename V = Action,
            std::enable_if_t<queryosity::query::template is_aggregation_v<V>,
                             bool> = false>
  auto result() const -> decltype(std::declval<V>().get_result());

  template <typename V = Action,
            std::enable_if_t<queryosity::query::template is_aggregation_v<V>,
                             bool> = false>
  auto operator->() const -> decltype(std::declval<V>().get_result()) {
    return this->result();
  }

  DEFINE_LAZY_UNARY_OP(logical_not, !)
  DEFINE_LAZY_UNARY_OP(minus, -)
  DEFINE_LAZY_BINARY_OP(equality, ==)
  DEFINE_LAZY_BINARY_OP(inequality, !=)
  DEFINE_LAZY_BINARY_OP(addition, +)
  DEFINE_LAZY_BINARY_OP(subtraction, -)
  DEFINE_LAZY_BINARY_OP(multiplication, *)
  DEFINE_LAZY_BINARY_OP(division, /)
  DEFINE_LAZY_BINARY_OP(logical_or, ||)
  DEFINE_LAZY_BINARY_OP(logical_and, &&)
  DEFINE_LAZY_BINARY_OP(greater_than, >)
  DEFINE_LAZY_BINARY_OP(less_than, <)
  DEFINE_LAZY_BINARY_OP(greater_than_or_equal_to, >=)
  DEFINE_LAZY_BINARY_OP(less_than_or_equal_to, <=)
  DEFINE_LAZY_SUBSCRIPT_OP()

protected:
  template <typename V = Action,
            std::enable_if_t<queryosity::query::template is_aggregation_v<V>,
                             bool> = false>
  void merge_results() const;

protected:
  std::vector<Action *> m_slots;
};

} // namespace queryosity

#include "column.h"
#include "lazy_varied.h"
#include "todo.h"

template <typename Action>
template <typename Derived>
queryosity::lazy<Action>::lazy(lazy<Derived> const &derived)
    : queryosity::dataflow::node(*derived.m_df) {
  this->m_slots.reserve(derived.m_slots.size());
  for (auto slot : derived.m_slots) {
    this->m_slots.push_back(static_cast<Action *>(slot));
  }
}

template <typename Action>
template <typename Derived>
queryosity::lazy<Action> &
queryosity::lazy<Action>::operator=(lazy<Derived> const &derived) {
  this->m_df = derived.m_df;
  this->m_slots.clear();
  this->m_slots.reserve(derived.m_slots.size());
  for (auto slot : derived.m_slots) {
    this->m_slots.push_back(static_cast<Action *>(slot));
  }
  return *this;
}

template <typename Action>
Action *queryosity::lazy<Action>::get_slot(unsigned int islot) const {
  return this->m_slots[islot];
}

template <typename Action>
unsigned int queryosity::lazy<Action>::concurrency() const {
  return this->m_slots.size();
}

template <typename Action>
void queryosity::lazy<Action>::set_variation(const std::string &, lazy) {
  // should never be called
  throw std::logic_error("cannot set variation to a nominal-only action");
}

template <typename Action> auto queryosity::lazy<Action>::nominal() -> lazy & {
  // this is nominal
  return *this;
}

template <typename Action>
auto queryosity::lazy<Action>::variation(const std::string &) -> lazy & {
  // propagation of variations must occur "transparently"
  return *this;
}

template <typename Action>
auto queryosity::lazy<Action>::nominal() const -> lazy const & {
  // this is nominal
  return *this;
}

template <typename Action>
auto queryosity::lazy<Action>::variation(const std::string &) const
    -> lazy const & {
  // propagation of variations must occur "transparently"
  return *this;
}

template <typename Action>
std::set<std::string> queryosity::lazy<Action>::list_variation_names() const {
  // no variations to list
  return std::set<std::string>();
}

template <typename Action>
bool queryosity::lazy<Action>::has_variation(const std::string &) const {
  // always false
  return false;
}

template <typename Action>
template <typename Col>
auto queryosity::lazy<Action>::filter(lazy<Col> const &col) const {
  if constexpr (std::is_base_of_v<selection::node, Action>) {
    return this->m_df->template _select<selection::cut>(*this, col);
  } else {
    static_assert(std::is_base_of_v<selection::node, Action>,
                  "filter must be called from a selection");
  }
}

template <typename Action>
template <typename Col>
auto queryosity::lazy<Action>::weight(lazy<Col> const &col) const {
  if constexpr (std::is_base_of_v<selection::node, Action>) {
    return this->m_df->template _select<selection::weight>(*this, col);
  } else {
    static_assert(std::is_base_of_v<selection::node, Action>,
                  "filter must be called from a selection");
  }
}

template <typename Action>
template <typename Col>
auto queryosity::lazy<Action>::filter(Col const &col) const {
  if constexpr (std::is_base_of_v<selection::node, Action>) {
    using varied_type = typename lazy<selection::node>::varied;
    auto syst = varied_type(*this->m_df, this->filter(col.nominal()));
    for (auto const &var_name : col.list_variation_names()) {
      syst.set_variation(var_name, this->filter(col.variation(var_name)));
    }
    return syst;
  } else {
    static_assert(std::is_base_of_v<selection::node, Action>,
                  "filter must be called from a selection");
  }
}

template <typename Action>
template <typename Col>
auto queryosity::lazy<Action>::weight(Col const &col) const {
  if constexpr (std::is_base_of_v<selection::node, Action>) {
    using varied_type = typename lazy<selection::node>::varied;
    auto syst = varied_type(*this->m_df, this->weight(col.nominal()));
    for (auto const &var_name : col.list_variation_names()) {
      syst.set_variation(var_name, this->weight(col.variation(var_name)));
    }
    return syst;
  } else {
    static_assert(std::is_base_of_v<selection::node, Action>,
                  "weight must be called from a selection");
  }
}

template <typename Action>
template <typename Expr, typename... Cols>
auto queryosity::lazy<Action>::filter(
    queryosity::column::expression<Expr> const &expr,
    Cols const &...cols) const {
  if constexpr (std::is_base_of_v<selection::node, Action>) {
    auto col = this->m_df->define(expr, cols...);
    return this->filter(col);
  } else {
    static_assert(std::is_base_of_v<selection::node, Action>,
                  "filter must be called from a selection");
  }
}

template <typename Action>
template <typename Expr, typename... Cols>
auto queryosity::lazy<Action>::weight(
    queryosity::column::expression<Expr> const &expr,
    Cols const &...cols) const {
  if constexpr (std::is_base_of_v<selection::node, Action>) {
    auto col = this->m_df->define(expr, cols...);
    return this->weight(col);
  } else {
    static_assert(std::is_base_of_v<selection::node, Action>,
                  "filter must be called from a selection");
  }
}

template <typename Action>
template <typename Agg>
auto queryosity::lazy<Action>::book(Agg &&agg) const {
  static_assert(std::is_base_of_v<selection::node, Action>,
                "book must be called from a selection");
  return agg.book(*this);
}

template <typename Action>
template <typename... Aggs>
auto queryosity::lazy<Action>::book(Aggs &&...aggs) const {
  static_assert(std::is_base_of_v<selection::node, Action>,
                "book must be called from a selection");
  return std::make_tuple((aggs.book(*this), ...));
}

template <typename Action>
template <
    typename V,
    std::enable_if_t<queryosity::query::template is_aggregation_v<V>, bool>>
auto queryosity::lazy<Action>::result() const
    -> decltype(std::declval<V>().get_result()) {
  this->m_df->analyze();
  this->merge_results();
  return this->get_slot(0)->get_result();
}

template <typename Action>
template <
    typename V,
    std::enable_if_t<queryosity::query::template is_aggregation_v<V>, bool> e>
void queryosity::lazy<Action>::merge_results() const {
  auto model = this->get_slot(0);
  const auto nslots = this->concurrency();
  if (nslots > 1 && !model->is_merged()) {
    std::vector<std::decay_t<decltype(model->get_result())>> results;
    results.reserve(nslots);
    for (size_t islot = 0; islot < nslots; ++islot) {
      results.push_back(this->get_slot(islot)->get_result());
    }
    model->set_merged_result(results);
  }
}