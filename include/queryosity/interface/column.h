#pragma once

#include <functional>
#include <memory>
#include <type_traits>

#include "action.h"

namespace queryosity {

/**
 * @brief Compute quantities of interest.
 */
namespace column {

class node : public action {};

//---------------------------------------------------
// view can actually report on the concrete data type
//---------------------------------------------------
template <typename T> class view {

public:
  using value_type = T;

public:
  template <typename U> class converted_from;

  template <typename U> class interface_of;

public:
  view() = default;
  virtual ~view() = default;

  virtual T const &value() const = 0;
  virtual T const *field() const;
};

//------------------------------------
// conversion between compatible types
//------------------------------------
template <typename To>
template <typename From>
class view<To>::converted_from : public view<To> {

public:
  converted_from(view<From> const &from);
  virtual ~converted_from() = default;

public:
  virtual const To &value() const override;

private:
  view<From> const *m_from;
  mutable To m_converted_from;
};

//------------------------------------------
// interface between inherited -> base type
//------------------------------------------
template <typename To>
template <typename From>
class view<To>::interface_of : public view<To> {

public:
  interface_of(view<From> const &from);
  virtual ~interface_of() = default;

public:
  virtual const To &value() const override;

private:
  view<From> const *m_impl;
};

template <typename T> class valued : public column::node, public view<T> {

public:
  using value_type = typename view<T>::value_type;

public:
  valued() = default;
  virtual ~valued() = default;

  virtual void initialize(unsigned int slot, unsigned long long begin,
                          unsigned long long end) override;
  virtual void execute(unsigned int slot, unsigned long long entry) override;
  virtual void finalize(unsigned int slot) override;
};

// costly to move around
template <typename T> class variable {

public:
  variable() = default;
  template <typename U> variable(view<U> const &val);
  virtual ~variable() = default;

  variable(variable &&) = default;
  variable &operator=(variable &&) = default;

  T const &value() const;
  T const *field() const;

protected:
  std::unique_ptr<const view<T>> m_view;
};

// easy to move around
template <typename T> class observable {

public:
  observable(variable<T> const &obs);
  virtual ~observable() = default;

  T const &value() const;
  T const *field() const;

  T const &operator*() const;
  T const *operator->() const;

protected:
  const variable<T> *m_var;
};

template <typename To, typename From>
std::unique_ptr<view<To>> view_as(view<From> const &from);

class computation;

template <typename T> class reader;

template <typename T> class fixed;

template <typename T> class calculation;

template <typename T> class definition;

template <typename T, typename U> class conversion;

template <typename T> class equation;

template <typename T> class representation;

template <typename T> class constant;

template <typename T> class expression;

template <typename T> class evaluator;

template <typename T>
constexpr std::true_type check_reader(typename column::reader<T> const &);
constexpr std::false_type check_reader(...);

template <typename T>
constexpr std::true_type check_fixed(typename column::fixed<T> const &);
constexpr std::false_type check_fixed(...);

template <typename T>
constexpr std::true_type
check_definition(typename column::definition<T> const &);
constexpr std::false_type check_definition(...);

template <typename T>
constexpr std::true_type check_equation(typename column::equation<T> const &);
constexpr std::false_type check_equation(...);

template <typename T>
constexpr std::true_type
check_representation(typename column::representation<T> const &);
constexpr std::false_type check_representation(...);

template <typename T>
constexpr bool is_reader_v =
    decltype(check_reader(std::declval<std::decay_t<T> const &>()))::value;

template <typename T>
constexpr bool is_fixed_v =
    decltype(check_fixed(std::declval<std::decay_t<T> const &>()))::value;

template <typename T>
constexpr bool is_definition_v =
    decltype(check_definition(std::declval<std::decay_t<T> const &>()))::value;

template <typename T>
constexpr bool is_equation_v =
    decltype(check_equation(std::declval<std::decay_t<T> const &>()))::value;

template <typename T>
constexpr bool is_representation_v = decltype(check_representation(
    std::declval<std::decay_t<T> const &>()))::value;

template <typename T> struct is_evaluator : std::false_type {};
template <typename T>
struct is_evaluator<column::evaluator<T>> : std::true_type {};

template <typename T> constexpr bool is_evaluatable_v = is_evaluator<T>::value;

template <typename Fn> struct deduce_equation;

template <typename Ret, typename... Args>
struct deduce_equation<std::function<Ret(Args...)>> {
  using type = column::equation<std::decay_t<Ret>(std::decay_t<Args>...)>;
};

template <typename Fn>
using equation_t = typename deduce_equation<
    typename column::expression<Fn>::function_type>::type;

template <typename T> using evaluated_t = typename T::evaluated_type;

template <typename T>
using value_t = std::decay_t<decltype(std::declval<T>().value())>;

} // namespace column

template <typename T>
constexpr bool is_column_v = std::is_base_of_v<column::node, T>;

} // namespace queryosity

template <typename T>
void queryosity::column::valued<T>::initialize(unsigned int, unsigned long long,
                                               unsigned long long) {}

template <typename T>
void queryosity::column::valued<T>::execute(unsigned int, unsigned long long) {}

template <typename T>
void queryosity::column::valued<T>::finalize(unsigned int) {}

template <typename T> T const *queryosity::column::view<T>::field() const {
  return &this->value();
}

template <typename To>
template <typename From>
queryosity::column::view<To>::converted_from<From>::converted_from(
    view<From> const &from)
    : m_from(&from) {}

template <typename To>
template <typename From>
const To &queryosity::column::view<To>::converted_from<From>::value() const {
  m_converted_from = std::move(m_from->value());
  return m_converted_from;
}

template <typename Base>
template <typename Impl>
queryosity::column::view<Base>::interface_of<Impl>::interface_of(
    view<Impl> const &from)
    : m_impl(&from) {}

template <typename Base>
template <typename Impl>
const Base &queryosity::column::view<Base>::interface_of<Impl>::value() const {
  return m_impl->value();
}

template <typename To, typename From>
std::unique_ptr<queryosity::column::view<To>>
queryosity::column::view_as(view<From> const &from) {
  static_assert(std::is_same_v<From, To> || std::is_base_of_v<To, From> ||
                    std::is_convertible_v<From, To>,
                "incompatible value types");
  if constexpr (std::is_same_v<From, To> || std::is_base_of_v<To, From>) {
    return std::make_unique<
        typename queryosity::column::view<To>::template interface_of<From>>(
        from);
  } else if constexpr (std::is_convertible_v<From, To>) {
    return std::make_unique<
        typename queryosity::column::view<To>::template converted_from<From>>(
        from);
  }
}

// --------
// variable
// --------

template <typename T>
template <typename U>
queryosity::column::variable<T>::variable(view<U> const &val)
    : m_view(view_as<T>(val)) {}

template <typename T> T const &queryosity::column::variable<T>::value() const {
  return m_view->value();
}

template <typename T> T const *queryosity::column::variable<T>::field() const {
  return m_view->field();
}

template <typename T>
queryosity::column::observable<T>::observable(const variable<T> &var)
    : m_var(&var) {}

template <typename T>
T const &queryosity::column::observable<T>::operator*() const {
  return m_var->value();
}

template <typename T>
T const *queryosity::column::observable<T>::operator->() const {
  return m_var->field();
}

template <typename T>
T const &queryosity::column::observable<T>::value() const {
  return m_var->value();
}

template <typename T>
T const *queryosity::column::observable<T>::field() const {
  return m_var->field();
}