#pragma once

#include "ana/term.h"

namespace ana
{

//------------------------------------------------------------------------------
// constant: value set manually
//------------------------------------------------------------------------------
template <typename Val>
class term<Val>::constant : public term<Val>
{

public:
  constant(const std::string& name, const Val& val);
  virtual ~constant() = default;

  const Val& value() const override;

protected:
  Val m_value;

};

template <typename Val>
class column::constant : public term<Val>::constant
{

public:
  constant(const std::string& name, const Val& val);
  virtual ~constant() = default;

};

}

template <typename Val>
ana::term<Val>::constant::constant(const std::string& name, const Val& val) :
  ana::term<Val>(name),
  m_value(val)
{}

template <typename Val>
ana::column::constant<Val>::constant(const std::string& name, const Val& val) :
  ana::term<Val>::constant(name, val)
{}

template <typename Val>
const Val& ana::term<Val>::constant::value() const
{
  return m_value;
}