#pragma once

#include <string>
#include <memory>
#include <functional>
#include <type_traits>

#include "ana/action.h"

namespace ana
{

class column : public action
{

public:
  template <typename T>
  class computation;

  template <typename T>
  class reader;

  template <typename T>
  class constant;

  template <typename T>
  class definition;

  template <typename T>
  class equation;

  template <typename T>
  class evaluator;

public: 
  column();
  virtual ~column() = default;

};

constexpr std::true_type check_column(const column&);
constexpr std::false_type check_column(...);
template <typename T> constexpr bool is_column_v = decltype(check_column(std::declval<T>()))::value;

template <typename T>
constexpr std::true_type check_column_reader(typename column::reader<T> const&);
constexpr std::false_type check_column_reader(...);
template <typename T> constexpr bool is_column_reader_v = decltype(check_column_reader(std::declval<T>()))::value;

template <typename T>
constexpr std::true_type check_column_constant(typename column::constant<T> const&);
constexpr std::false_type check_column_constant(...);
template <typename T> constexpr bool is_column_constant_v = decltype(check_column_constant(std::declval<T>()))::value;

template <typename T>
constexpr std::true_type check_column_equation(typename column::equation<T> const&);
constexpr std::false_type check_column_equation(...);
template <typename T> constexpr bool is_column_equation_v = decltype(check_column_equation(std::declval<T>()))::value;

template <typename T>
constexpr std::true_type check_column_definition(typename column::definition<T> const&);
constexpr std::false_type check_column_definition(...);
template <typename T> constexpr bool is_column_definition_v = decltype(check_column_definition(std::declval<T>()))::value;

//---------------------------------------------------
// cell can actually report on the concrete data type
//---------------------------------------------------
template <typename T>
class cell
{

public:
  using value_type = T;

public:
  template <typename U>
  class converted_from;

  template <typename U>
  class interface_to;

public:
  cell() = default;
  virtual ~cell() = default;

  virtual const T& value() const = 0;
  virtual const T* field() const;

};

//------------------------------------
// conversion between compatible types 
//------------------------------------
template <typename To>
template <typename From>
class cell<To>::converted_from : public cell<To>
{

public:
  converted_from(const cell<From>& from);
  virtual ~converted_from() = default;

public:
  virtual const To& value() const override;

private:
  const cell<From>* m_from;
	mutable To m_converted_from;

};

//------------------------------------------
// interface between inherited -> base type
//------------------------------------------
template <typename To>
template <typename From>
class cell<To>::interface_to : public cell<To>
{

public:
  interface_to(const cell<From>& from);
  virtual ~interface_to() = default;

public:
  virtual const To& value() const override;

private:
  const cell<From>* m_impl;

};

// term: get value of type T while ensured that it stays "updated"
template <typename T>
class term : public column, public cell<T>
{

public:
  using value_type = typename cell<T>::value_type;

public:
  class constant;

  class reader;

  class calculation;

  template <typename... Args>
  class calculated_with;

public:
  term() = default;
  virtual ~term() = default;

  virtual void initialize() override;
  virtual void execute() override;
  virtual void finalize() override;

};

// costly to move around
template <typename T>
class variable
{

public:
  variable();
  template <typename U>
  variable(const cell<U>& val);
  virtual ~variable() = default;

  const T& value() const;
  const T* field() const;

  explicit operator bool() const noexcept;

protected:
  std::shared_ptr<const cell<T>> m_val;

};

// easy to move around
template <typename T>
class observable
{

public:
  observable(const variable<T>& obs);
  virtual ~observable() = default;

  const T& value() const;
  const T* field() const;

  const T& operator*() const;
  const T* operator->() const;

protected:
  const variable<T>* m_var;

};

template <typename T>
class column::evaluator
{

public:
	using column_type = T;

public:
	template <typename... Args>
	evaluator(Args const&... args);
	~evaluator() = default;

	template <typename... Args>
  void set_constructor(Args const&... args);

	template <typename... Vals> 
	std::shared_ptr<T> evaluate_column( cell<Vals> const&... cols ) const;

protected:
	std::function<std::shared_ptr<T>()> m_make_shared_counter;

};

template <typename To, typename From>
std::shared_ptr<cell<To>> cell_as(const cell<From>& from);

template <typename T> using cell_value_t = std::decay_t<decltype(std::declval<T>().value())>;

}

template <typename T>
void ana::term<T>::initialize()
{}

template <typename T>
void ana::term<T>::execute()
{}

template <typename T>
void ana::term<T>::finalize()
{}

template <typename T>
const T* ana::cell<T>::field() const
{ 
  return &this->value();
}

template <typename To>
template <typename From>
ana::cell<To>::converted_from<From>::converted_from(const cell<From>& from) :
  ana::cell<To>(),
  m_from(&from)
{}

template <typename To>
template <typename From>
const To& ana::cell<To>::converted_from<From>::value() const
{
	m_converted_from = m_from->value();
	return m_converted_from;
}

template <typename Base>
template <typename Impl>
ana::cell<Base>::interface_to<Impl>::interface_to(const cell<Impl>& from) :
  cell<Base>(),
  m_impl(&from)
{}

template <typename Base>
template <typename Impl>
const Base& ana::cell<Base>::interface_to<Impl>::value() const
{
  return m_impl->value();
}

template <typename To, typename From>
std::shared_ptr<ana::cell<To>> ana::cell_as(const cell<From>& from)
{
  if constexpr(std::is_same_v<From,To> || std::is_base_of_v<From,To>) {
    return std::make_shared<typename ana::cell<To>::template interface_to<From>>(from);
  } else if constexpr(std::is_convertible_v<From,To>) {
    return std::make_shared<typename ana::cell<To>::template converted_from<From>>(from);
  } else {
    static_assert( std::is_same_v<From,To> || std::is_base_of_v<From,To> || std::is_convertible_v<From,To>, "incompatible value types" );
  }
}

// ---------
// evaluator
// ---------

template <typename T>
template <typename... Args>
ana::column::evaluator<T>::evaluator(Args const&... args) :
	m_make_shared_counter(std::bind([](Args const&... args){return std::make_shared<T>( args... );},  args... ))
{}

template <typename T>
template <typename... Args>
void ana::column::evaluator<T>::set_constructor(Args const&... args)
{
  m_make_shared_counter = std::bind([](Args const&... args){return std::make_shared<T>( args... );},  args... );
}

template <typename T>
template <typename... Vals>
std::shared_ptr<T> ana::column::evaluator<T>::evaluate_column(cell<Vals> const&... columns) const
{
  auto defn = m_make_shared_counter();

  defn->set_arguments(columns...);

  return defn;
}

// --------
// variable
// --------

template <typename T>
ana::variable<T>::variable() :
  m_val(nullptr)
{}

template <typename T>
template <typename U>
ana::variable<T>::variable(const cell<U>& val) :
  m_val(cell_as<T>(val))
{}

template <typename T>
const T& ana::variable<T>::value() const
{
  return m_val->value();
}

template <typename T>
const T* ana::variable<T>::field() const
{
  return m_val->field();
}

template <typename T>
ana::variable<T>::operator bool() const noexcept
{
  return m_val;
}

template <typename T>
ana::observable<T>::observable(const variable<T>& var) :
  m_var(&var)
{}

template <typename T>
const T& ana::observable<T>::operator*() const
{
  return m_var->value();
}

template <typename T>
const T* ana::observable<T>::operator->() const
{
  return m_var->field();
}

template <typename T>
const T& ana::observable<T>::value() const
{
  return m_var->value();
}

template <typename T>
const T* ana::observable<T>::field() const
{
  return m_var->field();
}