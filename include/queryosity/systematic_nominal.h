#pragma once

#include "systematic.h"

namespace queryosity {

template <typename T> class lazy;

namespace systematic {

template <typename Lzy> class nominal {

public:
  nominal(Lzy const &nom);
  virtual ~nominal() = default;

  auto get() const -> Lzy const &;

protected:
  Lzy const &m_nom;
};

} // namespace systematic

} // namespace queryosity

#include "lazy.h"

template <typename Lzy>
queryosity::systematic::nominal<Lzy>::nominal(Lzy const &nom) : m_nom(nom) {}

template <typename Lzy>
auto queryosity::systematic::nominal<Lzy>::get() const -> Lzy const & {
  return m_nom;
}