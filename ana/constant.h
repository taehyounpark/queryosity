#pragma once

#include "ana/column.h"

namespace ana
{

//------------------------------------------------------------------------------
// constant: value set manually
//------------------------------------------------------------------------------
template <typename Val>
class column<Val>::constant : public column<Val>
{

public:
  constant(const std::string& name, const Val& val);
  virtual ~constant() = default;

  const Val& value() const override;

protected:
  Val m_value;

};

}

template <typename Val>
ana::column<Val>::constant::constant(const std::string& name, const Val& val) :
  ana::column<Val>(name),
  m_value(val)
{}


template <typename Val>
const Val& ana::column<Val>::constant::value() const
{
  return m_value;
}