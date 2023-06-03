#pragma once

#include "column.h"

namespace ana {

//------------------------------------------------------------------------------
// constant: value set manually
//------------------------------------------------------------------------------
template <typename Val> class column::constant : public term<Val> {

public:
  constant(const Val &val);
  virtual ~constant() = default;

  const Val &value() const override;

protected:
  Val m_value;
};

} // namespace ana

template <typename Val>
ana::column::constant<Val>::constant(const Val &val) : m_value(val) {}

template <typename Val> const Val &ana::column::constant<Val>::value() const {
  return m_value;
}