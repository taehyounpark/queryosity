#pragma once

#include "column.h"

namespace ana {

//------------------------------------------------------------------------------
// fixed: value set manually
//------------------------------------------------------------------------------
template <typename Val> class column::fixed : public term<Val> {

public:
  fixed(Val const &val);
  template <typename... Args> fixed(Args &&...args);
  virtual ~fixed() = default;

  const Val &value() const override;

protected:
  Val m_value;
};

} // namespace ana

template <typename Val>
ana::column::fixed<Val>::fixed(Val const &val) : m_value(val) {}

template <typename Val>
template <typename... Args>
ana::column::fixed<Val>::fixed(Args &&...args)
    : m_value(std::forward<Args>(args)...) {}

template <typename Val> const Val &ana::column::fixed<Val>::value() const {
  return m_value;
}