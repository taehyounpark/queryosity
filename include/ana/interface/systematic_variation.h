#pragma once

/** @file */

#include <set>
#include <string>
#include <type_traits>

#include "dataflow.h"
#include "lazy.h"

namespace ana {

namespace systematic {

template <typename... Args> class variation {

public:
  variation(const std::string &name, Args... args)
      : m_name(name), m_args(args...) {}

  std::string const &name() const { return m_name; }
  std::tuple<Args...> const &args() const { return m_args; }

protected:
  std::string m_name;
  std::tuple<Args...> m_args;
};

} // namespace systematic

} // namespace ana