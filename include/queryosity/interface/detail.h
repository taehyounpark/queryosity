#pragma once

#include <type_traits>

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
                    detail::has_##op_name##_v<                                 \
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
                    detail::has_##op_name##_v<column::template value_t<V>>,    \
                bool> = false>                                                 \
  auto operator op_symbol() const {                                            \
    return this->m_df->define(queryosity::column::expression(                  \
                                  [](column::template value_t<V> const &me) {  \
                                    return (op_symbol me);                     \
                                  }),                                          \
                              *this);                                          \
  }

#define CHECK_FOR_INDEX_OP()                                                   \
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

#define DEFINE_LAZY_INDEX_OP()                                                 \
  template <typename Arg, typename V = Action,                                 \
            std::enable_if_t<                                                  \
                is_column_v<V> &&                                              \
                    detail::has_subscript_v<                                   \
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

#define DECLARE_LAZY_VARIED_BINARY_OP(op_symbol)                               \
  template <typename Arg>                                                      \
  auto operator op_symbol(Arg &&b) const->typename lazy<                       \
      typename decltype(std::declval<lazy<Act>>().operator op_symbol(          \
          std::forward<Arg>(b).nominal()))::action_type>::varied;

#define DEFINE_LAZY_VARIED_BINARY_OP(op_symbol)                                \
  template <typename Act>                                                      \
  template <typename Arg>                                                      \
  auto queryosity::lazy<Act>::varied::operator op_symbol(Arg &&b) const->      \
      typename lazy<                                                           \
          typename decltype(std::declval<lazy<Act>>().operator op_symbol(      \
              std::forward<Arg>(b).nominal()))::action_type>::varied {         \
    auto syst = typename lazy<                                                 \
        typename decltype(std::declval<lazy<Act>>().operator op_symbol(        \
            std::forward<Arg>(b).nominal()))::action_type>::                   \
        varied(this->nominal().operator op_symbol(                             \
            std::forward<Arg>(b).nominal()));                                  \
    for (auto const &var_name :                                                \
         systematic::list_all_variation_names(*this, std::forward<Arg>(b))) {  \
      syst.set_variation(var_name,                                             \
                         variation(var_name).operator op_symbol(               \
                             std::forward<Arg>(b).variation(var_name)));       \
    }                                                                          \
    return syst;                                                               \
  }

#define DECLARE_LAZY_VARIED_UNARY_OP(op_symbol)                                \
  template <typename V = Act,                                                  \
            std::enable_if_t<queryosity::is_column_v<V>, bool> = false>        \
  auto operator op_symbol() const->typename lazy<                              \
      typename decltype(std::declval<lazy<V>>().                               \
                        operator op_symbol())::action_type>::varied;
#define DEFINE_LAZY_VARIED_UNARY_OP(op_name, op_symbol)                        \
  template <typename Act>                                                      \
  template <typename V, std::enable_if_t<queryosity::is_column_v<V>, bool>>    \
  auto queryosity::lazy<Act>::varied::operator op_symbol() const->             \
      typename lazy<                                                           \
          typename decltype(std::declval<lazy<V>>().                           \
                            operator op_symbol())::action_type>::varied {      \
    auto syst =                                                                \
        typename lazy<typename decltype(std::declval<lazy<V>>().               \
                                        operator op_symbol())::action_type>::  \
            varied(this->nominal().operator op_symbol());                      \
    for (auto const &var_name : systematic::list_all_variation_names(*this)) { \
      syst.set_variation(var_name, variation(var_name).operator op_symbol());  \
    }                                                                          \
    return syst;                                                               \
  }

namespace queryosity {

namespace detail {

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
CHECK_FOR_INDEX_OP()

} // namespace detail
} // namespace queryosity