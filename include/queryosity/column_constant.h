#pragma once

#include "column.h"

namespace queryosity {

class dataflow;

template <typename Val> class lazy;

/**
 * @brief Define a constant column in dataflow.
 */
template <typename Val> class column::constant {

public:
  constant(Val const &val);
  ~constant() = default;

  auto _assign(dataflow &df) const -> lazy<column::valued<Val>>;

protected:
  Val m_val;
};

} // namespace queryosity

#include "dataflow.h"
#include "lazy.h"

template <typename Val>
queryosity::column::constant<Val>::constant(Val const &val) : m_val(val) {}

template <typename Val>
auto queryosity::column::constant<Val>::_assign(queryosity::dataflow &df) const
    -> lazy<column::valued<Val>> {
  return df._assign(this->m_val);
}