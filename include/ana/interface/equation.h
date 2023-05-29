#pragma once

#include <functional>

#include "definition.h"

namespace ana {

template <typename Ret, typename... Args>
class column::equation<Ret(Args...)> : public column::definition<Ret(Args...)> {

public:
  using vartuple_type = typename definition<Ret(Args...)>::vartuple_type;
  using function_type =
      std::function<std::decay_t<Ret>(std::decay_t<Args> const &...)>;

public:
  template <typename F> equation(F callable);
  virtual ~equation() = default;
  virtual Ret evaluate(observable<Args>... args) const override;

protected:
  vartuple_type m_arguments;
  function_type m_evaluate;
};

template <typename F>
auto make_equation(F expression)
    -> std::shared_ptr<column::template equation_t<F>>;

} // namespace ana

template <typename Ret, typename... Args>
template <typename F>
ana::column::equation<Ret(Args...)>::equation(F callable)
    : m_evaluate(callable) {}

template <typename Ret, typename... Args>
Ret ana::column::equation<Ret(Args...)>::evaluate(
    ana::observable<Args>... args) const {
  return this->m_evaluate(args.value()...);
}

template <typename F>
auto ana::make_equation(F expression)
    -> std::shared_ptr<ana::column::template equation_t<F>> {
  return std::make_shared<ana::column::template equation_t<F>>(expression);
}