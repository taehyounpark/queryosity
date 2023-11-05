#pragma once

/** @file */

#include <string>
#include <type_traits>

#include "dataflow.h"
#include "lazy.h"

namespace ana {

namespace systematic {

template <typename T> class nominal {

public:
  nominal(lazy<T> const &lazy) : m_lazy(lazy) {}

  lazy<T> get() const { return m_lazy; }

protected:
  lazy<T> m_lazy;
};

} // namespace systematic

} // namespace ana