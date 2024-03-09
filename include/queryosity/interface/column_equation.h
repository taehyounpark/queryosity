#pragma once

#include <functional>

#include "column_definition.h"

namespace queryosity {

template <typename Out, typename... Ins>
class column::equation<Out(Ins...)> : public column::definition<Out(Ins...)> {

public:
  using vartuple_type = typename definition<Out(Ins...)>::vartuple_type;
  using function_type =
      std::function<std::decay_t<Out>(std::decay_t<Ins> const &...)>;

public:
  template <typename Fn> equation(Fn fn);
  virtual ~equation() = default;

public:
  virtual Out evaluate(observable<Ins>... args) const override;

protected:
  vartuple_type m_arguments;
  function_type m_evaluate;
};

template <typename Fn>
auto make_equation(Fn fn) -> std::unique_ptr<column::template equation_t<Fn>>;

} // namespace queryosity

template <typename Out, typename... Ins>
template <typename Fn>
queryosity::column::equation<Out(Ins...)>::equation(Fn fn) : m_evaluate(fn) {}

template <typename Out, typename... Ins>
Out queryosity::column::equation<Out(Ins...)>::evaluate(
    observable<Ins>... args) const {
  return this->m_evaluate(args.value()...);
}

template <typename Fn>
auto queryosity::make_equation(Fn fn)
    -> std::unique_ptr<queryosity::column::template equation_t<Fn>> {
  return std::make_unique<queryosity::column::template equation_t<Fn>>(fn);
}