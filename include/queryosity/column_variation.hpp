#pragma once

#include <set>
#include <string>
#include <type_traits>

#include "column_definition.hpp"
#include "dataflow.hpp"
#include "lazy.hpp"

namespace queryosity {

namespace column {

template <typename Val> struct variation {

public:
  template <typename Col> variation(lazy<Col> const &var);
  ~variation() = default;

  variation(variation const &other) = default;            // copy constructor
  variation &operator=(variation const &other) = default; // copy assignment

  auto get() const -> lazy<valued<Val>> const &;

protected:
  lazy<valued<Val>> m_var;
};

} // namespace column

} // namespace queryosity

template <typename Val>
template <typename Col>
queryosity::column::variation<Val>::variation(queryosity::lazy<Col> const &var)
    : m_var(var.template to<Val>()) {}

template <typename Val>
auto queryosity::column::variation<Val>::get() const
    -> lazy<valued<Val>> const & {
  return m_var;
}
