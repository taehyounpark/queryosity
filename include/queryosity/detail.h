#pragma once

#include <type_traits>

// check for the existence of a native/custom binary operator
#define CHECK_FOR_BINARY_OP(op_name, op_symbol)                                \
  template <typename T, typename Arg, typename = void>                         \
  struct has_native_##op_name : std::false_type {};                            \
                                                                               \
  template <typename T, typename Arg>                                          \
  struct has_native_##op_name<                                                 \
      T, Arg,                                                                  \
      std::void_t<decltype(std::declval<T>() op_symbol std::declval<Arg>())>>  \
      : std::true_type {};                                                     \
                                                                               \
  template <typename T, typename Arg>                                          \
  auto operator op_symbol(const T &, const Arg &) -> priority_tag<0>;          \
                                                                               \
  template <typename T, typename Arg> struct has_custom_only_##op_name {       \
  private:                                                                     \
    template <typename U, typename V>                                          \
    static auto test(priority_tag<1>)                                          \
        -> decltype(std::declval<U>() op_symbol std::declval<V>(),             \
                    std::true_type());                                         \
                                                                               \
    template <typename, typename>                                              \
    static auto test(priority_tag<0>) -> std::false_type;                      \
                                                                               \
  public:                                                                      \
    static constexpr bool value =                                              \
        decltype(test<T, Arg>(priority_tag<1>()))::value &&                    \
        !has_native_##op_name<T, Arg>::value;                                  \
  };

#define DEFINE_LAZY_BINARY_OP(op_name, op_symbol)                              \
  template <typename Arg, typename V = Action,                                 \
            typename std::enable_if<                                           \
                queryosity::is_column_v<V> &&                                  \
                    queryosity::is_column_v<typename Arg::action_type> &&      \
                    (detail::has_native_##op_name<                             \
                         column::value_t<V>,                                   \
                         column::value_t<typename Arg::action_type>>::value || \
                     detail::has_custom_only_##op_name<                        \
                         column::value_t<V>,                                   \
                         column::value_t<typename Arg::action_type>>::value),  \
                bool>::type = true>                                            \
  auto operator op_symbol(Arg const &arg) const {                              \
    return this->m_df                                                          \
        ->define(queryosity::column::expression(                               \
            [](column::value_t<V> const &me,                                   \
               column::value_t<typename Arg::action_type> const &you) {        \
              return me op_symbol you;                                         \
            }))                                                                \
        .template evaluate(*this, arg);                                        \
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
  template <                                                                   \
      typename V = Action,                                                     \
      std::enable_if_t<queryosity::is_column_v<V> &&                           \
                           detail::has_##op_name##_v<column::value_t<V>>,      \
                       bool> = false>                                          \
  auto operator op_symbol() const {                                            \
    return this->m_df                                                          \
        ->define(queryosity::column::expression(                               \
            [](column::value_t<V> const &me) { return (op_symbol me); }))      \
        .template evaluate(*this);                                             \
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
  template <                                                                   \
      typename Arg, typename V = Action,                                       \
      std::enable_if_t<is_column_v<V> &&                                       \
                           detail::has_subscript_v<                            \
                               column::value_t<V>,                             \
                               column::value_t<typename Arg::action_type>>,    \
                       bool> = false>                                          \
  auto operator[](Arg const &arg) const {                                      \
    return this->m_df                                                          \
        ->define(queryosity::column::expression(                               \
            [](column::value_t<V> me,                                          \
               column::value_t<typename Arg::action_type> index) {             \
              return me[index];                                                \
            }))                                                                \
        .template evaluate(*this, arg);                                        \
  }

#define DECLARE_LAZY_VARIED_BINARY_OP(op_symbol)                               \
  template <                                                                   \
      typename Arg, typename V = Act,                                          \
      std::enable_if_t<queryosity::is_column_v<V> &&                           \
                           queryosity::is_column_v<typename Arg::action_type>, \
                       bool> = false>                                          \
  auto operator op_symbol(Arg const &b) const -> typename lazy<                \
      typename decltype(std::declval<lazy<Act>>().operator op_symbol(          \
          b.nominal()))::action_type>::varied;

#define DEFINE_LAZY_VARIED_BINARY_OP(op_symbol)                                \
  template <typename Act>                                                      \
  template <                                                                   \
      typename Arg, typename V,                                                \
      std::enable_if_t<queryosity::is_column_v<V> &&                           \
                           queryosity::is_column_v<typename Arg::action_type>, \
                       bool>>                                                  \
  auto queryosity::lazy<Act>::varied::operator op_symbol(Arg const &b) const   \
      -> typename lazy<                                                        \
          typename decltype(std::declval<lazy<Act>>().operator op_symbol(      \
              b.nominal()))::action_type>::varied {                            \
    auto syst = typename lazy<                                                 \
        typename decltype(std::declval<lazy<Act>>().operator op_symbol(        \
            b.nominal()))::action_type>::varied(this->nominal().               \
                                                operator op_symbol(            \
                                                    b.nominal()));             \
    for (auto const &var_name : systematic::get_variation_names(*this, b)) {   \
      syst.set_variation(var_name, variation(var_name).operator op_symbol(     \
                                       b.variation(var_name)));                \
    }                                                                          \
    return syst;                                                               \
  }

#define DECLARE_LAZY_VARIED_UNARY_OP(op_symbol)                                \
  template <typename V = Act,                                                  \
            std::enable_if_t<queryosity::is_column_v<V>, bool> = false>        \
  auto operator op_symbol() const -> typename lazy<                            \
      typename decltype(std::declval<lazy<V>>().                               \
                        operator op_symbol())::action_type>::varied;
#define DEFINE_LAZY_VARIED_UNARY_OP(op_name, op_symbol)                        \
  template <typename Act>                                                      \
  template <typename V, std::enable_if_t<queryosity::is_column_v<V>, bool>>    \
  auto queryosity::lazy<Act>::varied::operator op_symbol() const ->            \
      typename lazy<                                                           \
          typename decltype(std::declval<lazy<V>>().                           \
                            operator op_symbol())::action_type>::varied {      \
    auto syst =                                                                \
        typename lazy<typename decltype(std::declval<lazy<V>>().               \
                                        operator op_symbol())::action_type>::  \
            varied(this->nominal().operator op_symbol());                      \
    for (auto const &var_name : systematic::get_variation_names(*this)) {      \
      syst.set_variation(var_name, variation(var_name).operator op_symbol());  \
    }                                                                          \
    return syst;                                                               \
  }

namespace queryosity {

template <typename T> class lazy;
template <typename T> class todo;

template <typename U>
static constexpr std::true_type check_lazy(lazy<U> const &);
static constexpr std::false_type check_lazy(...) { return std::false_type{}; }
template <typename U>
static constexpr std::true_type check_todo(todo<U> const &);
static constexpr std::false_type check_todo(...) { return std::false_type{}; }

template <typename V>
static constexpr bool is_nominal_v =
    (decltype(check_lazy(std::declval<V>()))::value ||
     decltype(check_todo(std::declval<V>()))::value);
template <typename V> static constexpr bool is_varied_v = !is_nominal_v<V>;

template <typename... Args>
static constexpr bool has_no_variation_v = (is_nominal_v<Args> && ...);
template <typename... Args>
static constexpr bool has_variation_v = (is_varied_v<Args> || ...);

namespace detail {

// https://quuxplusone.github.io/blog/2021/07/09/priority-tag/
template <int I> struct priority_tag : priority_tag<I - 1> {};
template <> struct priority_tag<0> {};

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
CHECK_FOR_BINARY_OP(bitwise_or, &)
CHECK_FOR_BINARY_OP(bitwise_and, |)
CHECK_FOR_INDEX_OP()

} // namespace detail

} // namespace queryosity