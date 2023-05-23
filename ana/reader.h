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

  virtual void execute() override;

  virtual T const& read() const = 0;
  virtual T const& value() const override;

  std::string get_name() const;

protected:
  void update() const;
  void reset();

protected:
  const std::string m_name;
	mutable T const* m_addr;
  mutable bool m_updated;

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
void ana::term<T>::reader::execute()
{
  this->reset();
}

template <typename T>
T const& ana::term<T>::reader::value() const
{
  if (!this->m_updated) this->update();
  return *m_addr;
}

template <typename T>
void ana::term<T>::reader::update() const
{
  m_addr = &(this->read()); 
  m_updated = true;
}

template <typename T>
void ana::term<T>::reader::reset()
{
  m_updated = false;
}

template <typename T>
std::string ana::term<T>::reader::get_name() const
{
  return m_name;
}