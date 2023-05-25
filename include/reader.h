#pragma once

#include "ana/column.h"

namespace ana
{

template <typename T>
class term<T>::reader : public term<T>
{

public:
  reader(const std::string& name);
  virtual ~reader() = default;

  virtual const T& value() const override;
  void read(const T& val);

  std::string get_name() const;

protected:
  const std::string m_name;
	const T* m_addr;

};

template <typename T>
class column::reader : public term<T>::reader
{

public:
  reader(const std::string& name);
  virtual ~reader() = default;

};

}

template <typename T>
 ana::term<T>::reader::reader(const std::string& name) :
  term<T>(),
  m_name(name),
  m_addr(nullptr)
{}

template <typename T>
 ana::column::reader<T>::reader(const std::string& name) :
  term<T>::reader(name)
{}

template <typename T>
void ana::term<T>::reader::read(const T& val)
{
  m_addr = &val;
}

template <typename T>
const T& ana::term<T>::reader::value() const
{
  return *m_addr;
}

template <typename T>
std::string ana::term<T>::reader::get_name() const
{
  return m_name;
}