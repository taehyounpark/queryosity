#pragma once

#include <set>
#include <string>
#include <type_traits>

#include "dataflow.h"
#include "lazy.h"
#include "column_definition.h"

namespace queryosity {

template <typename... Args> class systematic::variation {

public:
  variation(const std::string &name, Args const&... args)
      : m_name(name), m_args(args...) {}

  std::string const &name() const { return m_name; }
  std::tuple<Args...> const &args() const { return m_args; }

  template <typename Arg>
  auto _vary_arg() const -> Arg {
    return std::apply([](Args const&... args){return Arg(args...);}, m_args);
  }

protected:
  std::string m_name;
  std::tuple<Args...> m_args;
};

template <typename Action> class systematic::variation<lazy<Action>> {

public:
  variation(const std::string &name, lazy<Action> const &var);
  virtual ~variation() = default;

  auto name() const -> std::string;
  auto get() const -> lazy<Action> const &;

protected:
  std::string m_name;
  lazy<Action> const &m_var;
};

} // namespace queryosity

template <typename Action>
queryosity::systematic::variation<queryosity::lazy<Action>>::variation(
    const std::string &name, queryosity::lazy<Action> const &var)
    : m_name(name), m_var(var) {}

template <typename Action>
auto queryosity::systematic::variation<queryosity::lazy<Action>>::name() const
    -> std::string {
  return m_name;
}

template <typename Action>
auto queryosity::systematic::variation<queryosity::lazy<Action>>::get() const
    -> queryosity::lazy<Action> const & {
  return m_var;
}
