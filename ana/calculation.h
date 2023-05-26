#pragma once

#include "ana/column.h"

namespace ana
{

/**
 * @brief Calculate a value for each dataset entry.
 * @details `term<T>::calculation` is a `term<T>` abstract class 
 * that holds a value of type `T` to be updated for each entry.
 * Its action execution marks itself as "not updated" such that 
 * if and when its value is requested for the first time in an entry, 
 * it calculates the value and stores it for subsequent accesses in the entry.
 * `T` must be *CopyConstructible* and *CopyAssignable*.
*/
template <typename T>
class term<T>::calculation : public term<T>
{

public:
	calculation() = default;
  virtual ~calculation() = default;

public:
  virtual const T& value() const override;

  virtual T calculate() const = 0;

	virtual void initialize() override;
	virtual void execute()    override final;
	virtual void finalize()   override;

protected:
  void update() const;
  void reset() const;

protected:
  mutable T    m_value;
  mutable bool m_updated;

};

}

template <typename T>
const T& ana::term<T>::calculation::value() const
{
  if (!m_updated) this->update();
  return m_value;
}

template <typename T>
void ana::term<T>::calculation::update() const
{
  m_value = this->calculate();
  m_updated = true;
}

template <typename T>
void ana::term<T>::calculation::reset() const
{
  m_updated = false;
}

template <typename T>
void ana::term<T>::calculation::initialize()
{}

template <typename T>
void ana::term<T>::calculation::execute()
{ 
  this->reset();
}

template <typename T>
void ana::term<T>::calculation::finalize()
{}