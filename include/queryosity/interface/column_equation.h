#pragma once

#include <functional>

#include "column_definition.h"

namespace queryosity {

template <typename Ret, typename... Vals>
class column::equation<Ret(Vals...)> : public column::definition<Ret(Vals...)> {

public:
  using vartuple_type = typename definition<Ret(Vals...)>::vartuple_type;
  using function_type =
      std::function<std::decay_t<Ret>(std::decay_t<Vals> const &...)>;

public:
  template <typename Fn> equation(Fn fn);
  virtual ~equation() = default;

public:
  virtual Ret evaluate(observable<Vals>... args) const override;

protected:
  vartuple_type m_arguments;
  function_type m_evaluate;
};

template <typename Fn>
auto make_equation(Fn fn) -> std::unique_ptr<column::template equation_t<Fn>>;

} // namespace queryosity

template <typename Ret, typename... Vals>
template <typename Fn>
queryosity::column::equation<Ret(Vals...)>::equation(Fn fn) : m_evaluate(fn) {}

template <typename Ret, typename... Vals>
Ret queryosity::column::equation<Ret(Vals...)>::evaluate(
    observable<Vals>... args) const {
  return this->m_evaluate(args.value()...);
}

template <typename Fn>
auto queryosity::make_equation(Fn fn)
    -> std::unique_ptr<queryosity::column::template equation_t<Fn>> {
  return std::make_unique<queryosity::column::template equation_t<Fn>>(fn);
}