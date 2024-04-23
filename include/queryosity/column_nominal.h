#pragma once

#include "column.h"

namespace queryosity {

template <typename> class lazy;

namespace column {

template <typename Col> class nominal {

public:
  using column_type = valued<value_t<Col>>;

public:
  nominal(lazy<Col> const &nom);
  ~nominal() = default;

  auto get() const -> lazy<valued<value_t<Col>>> const &;

protected:
  lazy<valued<value_t<Col>>> const &m_nom;
};

} // namespace systematic

} // namespace queryosity

#include "lazy.h"

template <typename Col>
queryosity::column::nominal<Col>::nominal(lazy<Col> const &nom) : m_nom(nom) {}

template <typename Col>
auto queryosity::column::nominal<Col>::get() const -> lazy<valued<value_t<Col>>> const & {
  return m_nom;
}