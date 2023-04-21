#pragma once

#include <string>
#include <memory>
#include <type_traits>

#include "ana/action.h"
#include "ana/concurrent.h"

namespace ana
{

class column : public action
{

public:
  template <typename Dat>
  class computation;

  template <typename T>
  class reader;

  template <typename T>
  class constant;

  template <typename T>
  class calculation;

  template <typename T>
  class definition;

  template <typename T>
  class equation;

public: 
  column(const std::string& name);
  virtual ~column() = default;

};

template <typename T>
class cell;

template <typename T>
class variable;

template <typename T>
class observable;

//------------------------------------------------------------------------------
// cell<T>: value can be "observed" through value()
//------------------------------------------------------------------------------
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

template <typename T>
using value_t = typename cell<T>::value_type;

//------------------------------------------------------------------------------
// converted_from 
//------------------------------------------------------------------------------
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

//------------------------------------------------------------------------------
// interface_to
//------------------------------------------------------------------------------
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

template <typename To, typename From>
std::shared_ptr<cell<To>> cell_as(const cell<From>& from);

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


}

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