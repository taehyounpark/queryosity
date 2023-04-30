#pragma once

#include "ana/column.h"

namespace ana
{

//------------------------------------------------------------------------------
// constant: value set manually
//------------------------------------------------------------------------------
template <typename Val>
class term<Val>::constant : public term<Val>
{

public:
  constant(const Val& val);
  virtual ~constant() = default;

  const Val& value() const override;

protected:
  Val m_value;

};

template <typename Val>
class column::constant : public term<Val>::constant
{

public:
  constant(const Val& val);
  virtual ~constant() = default;

};

}

template <typename Val>
ana::term<Val>::constant::constant(const Val& val) :
  ana::term<Val>(),
  m_value(val)
{}

template <typename Val>
ana::column::constant<Val>::constant( const Val& val) :
  ana::term<Val>::constant(val)
{}

template <typename Val>
const Val& ana::term<Val>::constant::value() const
{
  return m_value;
}