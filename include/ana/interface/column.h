#pragma once

#include <functional>
#include <memory>
#include <string>
#include <type_traits>

#include "action.h"

namespace ana {

template <typename T> struct is_callable {
  using yes = bool;
  using no = int;
  template <typename C> static yes check_callable(decltype(&C::operator()));
  template <typename C> static no check_callable(...);
  enum { value = sizeof(check_callable<T>(0)) == sizeof(yes) };
};

/**
 * @brief Determine whether a type is callable, i.e. has a valid `operator()`.
 */
template <typename T> constexpr bool is_callable_v = is_callable<T>::value;

class column;

template <typename T> constexpr bool is_column_v = std::is_base_of_v<column, T>;

class column : public action {

public:
  template <typename T> class computation;

  template <typename T> class reader;

  template <typename T> class constant;

  template <typename T> class definition;

  template <typename T> class equation;

  template <typename T> class evaluator;

  template <typename T> class representation;

  template <typename T> class representor;

public:
  column() = default;
  virtual ~column() = default;

public:
  template <typename T>
  static constexpr std::true_type
  check_reader(typename column::reader<T> const &);
  static constexpr std::false_type check_reader(...);

  template <typename T>
  static constexpr std::true_type
  check_constant(typename column::constant<T> const &);
  static constexpr std::false_type check_constant(...);

  template <typename T>
  static constexpr std::true_type
  check_equation(typename column::equation<T> const &);
  static constexpr std::false_type check_equation(...);

  template <typename T>
  static constexpr std::true_type
  check_representation(typename column::representation<T> const &);
  static constexpr std::false_type check_representation(...);

  template <typename T>
  static constexpr std::true_type
  check_definition(typename column::definition<T> const &);
  static constexpr std::false_type check_definition(...);

  template <typename T> struct is_evaluator : std::false_type {};
  template <typename T>
  struct is_evaluator<column::evaluator<T>> : std::true_type {};

  template <typename T, typename = void> struct evaluator_traits;
  template <typename T>
  struct evaluator_traits<T, typename std::enable_if_t<ana::is_column_v<T>>> {
    using evaluator_type = typename ana::column::template evaluator<T>;
  };

  template <typename F> struct equation_traits {
    using equation_type = typename equation_traits<decltype(
        std::function{std::declval<F>()})>::equation_type;
  };

  template <typename Ret, typename... Args>
  struct equation_traits<std::function<Ret(Args...)>> {
    using equation_type =
        typename column::equation<std::decay_t<Ret>(std::decay_t<Args>...)>;
  };

  template <typename F>
  using equation_t = typename equation_traits<F>::equation_type;

  template <typename F>
  struct evaluator_traits<F, typename std::enable_if_t<!ana::is_column_v<F> &&
                                                       ana::is_callable_v<F>>> {
    using evaluator_type = typename ana::column::template evaluator<
        ana::column::template equation_t<F>>;
  };

  template <typename T>
  static constexpr bool is_reader_v =
      decltype(check_reader(std::declval<std::decay_t<T> const &>()))::value;

  template <typename T>
  static constexpr bool is_constant_v =
      decltype(check_constant(std::declval<std::decay_t<T> const &>()))::value;

  template <typename T>
  static constexpr bool is_definition_v = decltype(check_definition(
      std::declval<std::decay_t<T> const &>()))::value;

  template <typename T>
  static constexpr bool is_equation_v =
      decltype(check_equation(std::declval<std::decay_t<T> const &>()))::value;

  template <typename T>
  static constexpr bool is_representation_v = decltype(check_representation(
      std::declval<std::decay_t<T> const &>()))::value;

  template <typename T>
  static constexpr bool is_evaluator_v = is_evaluator<T>::value;

  template <typename T>
  using evaluator_t = typename evaluator_traits<T>::evaluator_type;

  template <typename T> using evaluated_t = typename T::evaluated_type;
};

//---------------------------------------------------
// cell can actually report on the concrete data type
//---------------------------------------------------
template <typename T> class cell {

public:
  using value_type = T;

public:
  template <typename U> class conversion_of;

  template <typename U> class interface_of;

public:
  cell() = default;
  virtual ~cell() = default;

  virtual const T &value() const = 0;
  virtual const T *field() const;
};

//------------------------------------
// conversion between compatible types
//------------------------------------
template <typename To>
template <typename From>
class cell<To>::conversion_of : public cell<To> {

public:
  conversion_of(const cell<From> &from);
  virtual ~conversion_of() = default;

public:
  virtual const To &value() const override;

private:
  const cell<From> *m_from;
  mutable To m_conversion_of;
};

//------------------------------------------
// interface between inherited -> base type
//------------------------------------------
template <typename To>
template <typename From>
class cell<To>::interface_of : public cell<To> {

public:
  interface_of(const cell<From> &from);
  virtual ~interface_of() = default;

public:
  virtual const To &value() const override;

private:
  const cell<From> *m_impl;
};

// term: get value of type T while ensured that it stays "updated"
template <typename T> class term : public column, public cell<T> {

public:
  using value_type = typename cell<T>::value_type;

public:
  class constant;

  class reader;

  class calculation;

  template <typename... Args> class calculation_of;

  template <typename... Args> class representation_of;

public:
  term() = default;
  virtual ~term() = default;

  virtual void initialize() override;
  virtual void execute() override;
  virtual void finalize() override;
};

// costly to move around
template <typename T> class variable {

public:
  variable() = default;
  template <typename U> variable(const cell<U> &val);
  virtual ~variable() = default;

  const T &value() const;
  const T *field() const;

protected:
  std::shared_ptr<const cell<T>> m_val;
};

// easy to move around
template <typename T> class observable {

public:
  observable(const variable<T> &obs);
  virtual ~observable() = default;

  const T &value() const;
  const T *field() const;

  const T &operator*() const;
  const T *operator->() const;

protected:
  const variable<T> *m_var;
};

template <typename T> class column::evaluator {

public:
  using evaluated_type = T;

public:
  template <typename... Args> evaluator(Args const &...args);
  ~evaluator() = default;

  template <typename... Args> void set_constructor(Args const &...args);

  template <typename... Vals>
  std::shared_ptr<T> evaluate_column(cell<Vals> const &...cols) const;

protected:
  std::function<std::shared_ptr<T>()> m_make_shared;
};

template <typename To, typename From>
std::shared_ptr<cell<To>> cell_as(const cell<From> &from);

template <typename T>
using cell_value_t = std::decay_t<decltype(std::declval<T>().value())>;

} // namespace ana

template <typename T> void ana::term<T>::initialize() {}

template <typename T> void ana::term<T>::execute() {}

template <typename T> void ana::term<T>::finalize() {}

template <typename T> const T *ana::cell<T>::field() const {
  return &this->value();
}

template <typename To>
template <typename From>
ana::cell<To>::conversion_of<From>::conversion_of(const cell<From> &from)
    : m_from(&from) {}

template <typename To>
template <typename From>
const To &ana::cell<To>::conversion_of<From>::value() const {
  m_conversion_of = m_from->value();
  return m_conversion_of;
}

template <typename Base>
template <typename Impl>
ana::cell<Base>::interface_of<Impl>::interface_of(const cell<Impl> &from)
    : m_impl(&from) {}

template <typename Base>
template <typename Impl>
const Base &ana::cell<Base>::interface_of<Impl>::value() const {
  return m_impl->value();
}

template <typename To, typename From>
std::shared_ptr<ana::cell<To>> ana::cell_as(const cell<From> &from) {
  if constexpr (std::is_same_v<From, To> || std::is_base_of_v<From, To>) {
    return std::make_shared<
        typename ana::cell<To>::template interface_of<From>>(from);
  } else if constexpr (std::is_convertible_v<From, To>) {
    return std::make_shared<
        typename ana::cell<To>::template conversion_of<From>>(from);
  } else {
    static_assert(std::is_same_v<From, To> || std::is_base_of_v<From, To> ||
                      std::is_convertible_v<From, To>,
                  "incompatible value types");
  }
}

// ---------
// evaluator
// ---------

template <typename T>
template <typename... Args>
ana::column::evaluator<T>::evaluator(Args const &...args)
    : m_make_shared(std::bind(
          [](Args const &...args) { return std::make_shared<T>(args...); },
          args...)) {}

template <typename T>
template <typename... Args>
void ana::column::evaluator<T>::set_constructor(Args const &...args) {
  m_make_shared = std::bind(
      [](Args const &...args) { return std::make_shared<T>(args...); },
      args...);
}

template <typename T>
template <typename... Vals>
std::shared_ptr<T>
ana::column::evaluator<T>::evaluate_column(cell<Vals> const &...columns) const {
  auto defn = m_make_shared();

  defn->set_arguments(columns...);

  return defn;
}

// --------
// variable
// --------

template <typename T>
template <typename U>
ana::variable<T>::variable(const cell<U> &val) : m_val(cell_as<T>(val)) {}

template <typename T> const T &ana::variable<T>::value() const {
  return m_val->value();
}

template <typename T> const T *ana::variable<T>::field() const {
  return m_val->field();
}

template <typename T>
ana::observable<T>::observable(const variable<T> &var) : m_var(&var) {}

template <typename T> const T &ana::observable<T>::operator*() const {
  return m_var->value();
}

template <typename T> const T *ana::observable<T>::operator->() const {
  return m_var->field();
}

template <typename T> const T &ana::observable<T>::value() const {
  return m_var->value();
}

template <typename T> const T *ana::observable<T>::field() const {
  return m_var->field();
}