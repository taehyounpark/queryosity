#pragma once

#include "column.h"

namespace ana {

//------------------------------------------------------------------------------
// constant: value set manually
//------------------------------------------------------------------------------
template <typename Val> class column::constant : public term<Val> {

public:
  constant(Val const &val);
  template <typename... Args> constant(Args &&...args);
  virtual ~constant() = default;

  const Val &value() const override;

protected:
  Val m_value;
};

} // namespace ana

template <typename Val>
ana::column::constant<Val>::constant(Val const &val) : m_value(val) {}

template <typename Val>
template <typename... Args>
ana::column::constant<Val>::constant(Args &&...args)
    : m_value(std::forward<Args>(args)...) {}

template <typename Val> const Val &ana::column::constant<Val>::value() const {
  return m_value;
}