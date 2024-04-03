#pragma once

#include <set>
#include <string>
#include <type_traits>

#include "dataflow.h"
#include "lazy.h"
#include "column_definition.h"

namespace queryosity {

template <typename Lazy> class systematic::variation {

public:
  variation(const std::string &name, Lazy const &var);
  virtual ~variation() = default;

  auto name() const -> std::string;
  auto get() const -> Lazy const &;

protected:
  std::string m_name;
  Lazy m_var;
};

} // namespace queryosity

template <typename Lazy>
queryosity::systematic::variation<Lazy>::variation(
    const std::string &name, Lazy const &var)
    : m_name(name), m_var(var) {}

template <typename Lazy>
auto queryosity::systematic::variation<Lazy>::name() const
    -> std::string {
  return m_name;
}

template <typename Lazy>
auto queryosity::systematic::variation<Lazy>::get() const
    -> Lazy const & {
  return m_var;
}
