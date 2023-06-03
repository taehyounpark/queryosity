#pragma once

#include <functional>

#include "definition.h"

namespace ana {

template <typename Ret, typename... Vals>
class column::equation<Ret(Vals...)> : public column::definition<Ret(Vals...)> {

public:
  using vartuple_type = typename definition<Ret(Vals...)>::vartuple_type;
  using function_type =
      std::function<std::decay_t<Ret>(std::decay_t<Vals> const &...)>;

public:
  template <typename F> equation(F callable);
  template <typename F, typename... Args> equation(F callable, Args &&...args);
  virtual ~equation() = default;

public:
  virtual Ret evaluate(observable<Vals>... args) const override;

protected:
  vartuple_type m_arguments;
  function_type m_evaluate;
};

template <typename F>
auto make_equation(F expression)
    -> std::unique_ptr<column::template equation_t<F>>;

} // namespace ana

template <typename Ret, typename... Vals>
template <typename F>
ana::column::equation<Ret(Vals...)>::equation(F callable)
    : m_evaluate(callable) {}

template <typename Ret, typename... Vals>
template <typename F, typename... Args>
ana::column::equation<Ret(Vals...)>::equation(F callable, Args &&...args)
    : equation(callable),
      definition<Ret(Vals...)>(std::forward<Args>(args)...) {}

template <typename Ret, typename... Vals>
Ret ana::column::equation<Ret(Vals...)>::evaluate(
    ana::observable<Vals>... args) const {
  return this->m_evaluate(args.value()...);
}

template <typename F>
auto ana::make_equation(F expression)
    -> std::unique_ptr<ana::column::template equation_t<F>> {
  return std::make_unique<ana::column::template equation_t<F>>(expression);
}