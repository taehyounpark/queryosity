#pragma once

#include <set>
#include <string>
#include <type_traits>

#include "dataflow.h"
#include "lazy.h"
#include "column_definition.h"

namespace queryosity {

namespace column {

template <typename Val> struct variation {

public:
  template <typename Col>
  variation(lazy<Col> const& var);
  ~variation() = default;

  auto get() const -> lazy<valued<Val>> const &;

protected:
  lazy<valued<Val>> m_var;
};

}

} // namespace queryosity

template <typename Val>
template <typename Col>
queryosity::column::variation<Val>::variation(queryosity::lazy<Col> const& var) : m_var(var.template to<Val>()) {}

template <typename Val>
auto queryosity::column::variation<Val>::get() const
    -> lazy<valued<Val>> const & {
  return m_var;
}
