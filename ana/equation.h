#pragma once

#include <memory>
#include <tuple>
#include <functional>

#include "ana/definition.h"

namespace ana
{

template <typename Ret, typename... Args>
auto make_equation(std::function<Ret(Args...)> func) -> std::shared_ptr<column::equation<std::decay_t<Ret>(std::decay_t<Args>...)>>;
template <typename Fn> using equation_t = typename decltype(make_equation(std::function(std::declval<Fn>())))::element_type;

template <typename Ret, typename... Args>
class column::equation<Ret(Args...)> : public column::definition<Ret(Args...)>
{

public:
  using argtuple_type = typename definition<Ret(Args...)>::argtup_type;
  using evalfunc_type = std::function<Ret(Args...)>;

public:
  equation(std::function<Ret(Args...)>);
	virtual ~equation() = default;

  // convert each input argument type
  // template <typename... UArgs>
  // void set_arguments(cell<UArgs> const&... args);

  virtual Ret evaluate(observable<Args>... args) const override;

protected:
	argtuple_type m_arguments;
	evalfunc_type m_evaluate;

};

}

template <typename Ret, typename... Args>
ana::column::equation<Ret(Args...)>::equation(std::function<Ret(Args...)> expression) :
	definition<Ret(Args...)>(),
  m_evaluate(expression)
{
  m_evaluate = expression;
}

// user-defined expression with input arguments
template <typename Ret, typename... Args>
Ret ana::column::equation<Ret(Args...)>::evaluate(ana::observable<Args>... args) const
{
  return this->m_evaluate(args.value()...);
}

template <typename Ret, typename... Args>
auto ana::make_equation(std::function<Ret(Args...)> func) -> std::shared_ptr<ana::column::equation<std::decay_t<Ret>(std::decay_t<Args>...)>>
{
	auto eqn = std::make_shared<column::equation<std::decay_t<Ret>(std::decay_t<Args>...)>>(func);
	return eqn;
}