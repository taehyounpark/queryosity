#pragma once

#include <string>
#include <memory>
#include <type_traits>

#include "ana/action.h"
#include "ana/concurrent.h"

namespace ana
{

class variable : public action
{

public:
  template <typename Dat>
  class computation;

public: 
  variable(const std::string& name);
  virtual ~variable() = default;

};

//------------------------------------------------------------------------------
// cell<T>: value can be "observed" through value()
//------------------------------------------------------------------------------
template <typename T>
class cell
{

public:
  using value_type = T;

public:
  template <typename From>
  class conversion;

  template <typename From>
  class interface;

public:
  cell() = default;
  virtual ~cell() = default;

  virtual const T& value() const = 0;
  virtual const T* field() const;

};

template <typename T>
struct get_value
{
  typedef typename std::decay<decltype(std::declval<T>().value())>::type type;
};

template <typename T>
using get_value_t = typename get_value<T>::type;

//------------------------------------------------------------------------------
// conversion 
//------------------------------------------------------------------------------
template <typename To>
template <typename From>
class cell<To>::conversion : public cell<To>
{

public:
  conversion(const cell<From>& from);
  virtual ~conversion() = default;

public:
  virtual const To& value() const override;

private:
  const cell<From>& m_from;
	mutable To m_conversion;

};

//------------------------------------------------------------------------------
// interface
//------------------------------------------------------------------------------
template <typename To>
template <typename From>
class cell<To>::interface : public cell<To>
{

public:
  interface(const cell<From>& from);
  virtual ~interface() = default;

public:
  virtual const To& value() const override;

private:
  const cell<From>& m_impl;

};

template <typename T>
class observable
{

public:
  observable(const cell<T>& valuable);
  virtual ~observable() = default;

  const T& value() const;
  const T* field() const;

  const T& operator()() const;
  const T* operator->() const;

protected:
  const cell<T>& m_valuable;

};


template <typename To, typename From>
std::shared_ptr<cell<To>> value_as(const cell<From>& from)
{
  if constexpr(std::is_same_v<To,From> || std::is_base_of_v<To,From>) {
    return std::make_shared<typename ana::cell<To>::template interface<From>>(from);
  } else if constexpr(std::is_convertible_v<From,To>) {
    return std::make_shared<typename ana::cell<To>::template conversion<From>>(from);
  } else {
    static_assert( std::is_same_v<From,To> || std::is_base_of_v<From,To> || std::is_convertible_v<From,To>, "incompatible data types" );
  }
}

}

template <typename T>
const T* ana::cell<T>::field() const
{ 
  return &this->value();
}

template <typename To>
template <typename From>
ana::cell<To>::conversion<From>::conversion(const cell<From>& from) :
  ana::cell<To>(),
  m_from(from)
{}

template <typename To>
template <typename From>
const To& ana::cell<To>::conversion<From>::value() const
{
	m_conversion = m_from.value();
	return m_conversion;
}

template <typename To>
template <typename From>
ana::cell<To>::interface<From>::interface(const cell<From>& from) :
  cell<To>(),
  m_impl(from)
{}

template <typename To>
template <typename From>
const To& ana::cell<To>::interface<From>::value() const
{
  return m_impl.value();
}

template <typename T>
ana::observable<T>::observable(const cell<T>& valuable) :
  m_valuable(valuable)
{}

template <typename T>
const T& ana::observable<T>::value() const
{
  return m_valuable.value();
}

template <typename T>
const T* ana::observable<T>::field() const
{
  return m_valuable.field();
}

template <typename T>
const T& ana::observable<T>::operator()() const
{
  return m_valuable.value();
}

template <typename T>
const T* ana::observable<T>::operator->() const
{
  return m_valuable.field();
}