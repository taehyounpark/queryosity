#pragma once

#include "column.h"

namespace ana {

class dataflow;

template <typename Val> class lazy;

template <typename Val> class column::constant {

public:
  constant(Val const &val);
  ~constant() = default;

  auto _assign(dataflow &df) const -> lazy<column::fixed<Val>>;

protected:
  Val m_val;
};

} // namespace ana

#include "dataflow.h"
#include "lazy.h"

template <typename Val>
ana::column::constant<Val>::constant(Val const &val) : m_val(val) {}

template <typename Val>
auto ana::column::constant<Val>::_assign(ana::dataflow &df) const
    -> lazy<column::fixed<Val>> {
  return df._assign(this->m_val);
}