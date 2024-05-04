#pragma once

#include "column.hpp"

namespace queryosity {

class dataflow;

template <typename Val> class lazy;

namespace column {

/**
 * @ingroup api
 * @brief Argument to define a column of constant value in the dataflow.
 * @tparam Val Data type of the constant value.
 */
template <typename Val> struct constant {

public:
  /**
   * @brief Argument constructor.
   * @param[in] val Constant value.
   */
  constant(Val const &val);
  ~constant() = default;

  auto _assign(dataflow &df) const -> lazy<column::valued<Val>>;

protected:
  Val m_val;
};

} // namespace column

} // namespace queryosity

#include "dataflow.hpp"
#include "lazy.hpp"

template <typename Val>
queryosity::column::constant<Val>::constant(Val const &val) : m_val(val) {}

template <typename Val>
auto queryosity::column::constant<Val>::_assign(queryosity::dataflow &df) const
    -> lazy<column::valued<Val>> {
  return df._assign(this->m_val);
}