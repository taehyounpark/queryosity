#pragma once

#include "column.hpp"

namespace queryosity {

template <typename> class lazy;

namespace column {

template <typename Col> struct nominal {

public:
  using column_type = valued<value_t<Col>>;

public:
  nominal(lazy<Col> const &nom);
  ~nominal() = default;

  auto get() const -> lazy<valued<value_t<Col>>> const &;

protected:
  lazy<valued<value_t<Col>>> const &m_nom;
};

} // namespace column

} // namespace queryosity

#include "lazy.hpp"

template <typename Col>
queryosity::column::nominal<Col>::nominal(lazy<Col> const &nom) : m_nom(nom) {}

template <typename Col>
auto queryosity::column::nominal<Col>::get() const
    -> lazy<valued<value_t<Col>>> const & {
  return m_nom;
}