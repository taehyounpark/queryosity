#pragma once

#include <memory>
#include <tuple>
#include <functional>

#include "ana/definition.h"

namespace ana
{

// a shortcut for column::definition<Ret(Args...)>
// where an std::function<Ret(Args const&...)> is used as the evaluation
template <typename Ret, typename... Args>
class column::equation<Ret(Args...)> : public column::definition<Ret(Args...)>
{

public:
  using argtuple_type = typename definition<Ret(Args...)>::argtup_type;
  using evalfunc_type = std::function<Ret(std::decay_t<Args> const&...)>;

public:
  equation(std::function<Ret(Args const&...)>);
	virtual ~equation() = default;

  virtual Ret evaluate(observable<Args>... args) const override;

protected:
	argtuple_type m_arguments;
	evalfunc_type m_evaluate;

};

template <typename T>
struct eqn_traits {};
template <typename Ret, typename... Args>
struct eqn_traits<std::function<Ret(Args...)>> { using equation_type = column::equation<std::decay_t<Ret>(std::decay_t<Args>...)>; };

template <typename Fn>
using equation_t = typename eqn_traits<Fn>::equation_type;

template <typename Ret, typename... Args>
auto make_equation(std::function<Ret(Args...)> func) -> std::shared_ptr<equation_t<std::function<Ret(Args...)>>>;

}

template <typename Ret, typename... Args>
ana::column::equation<Ret(Args...)>::equation(std::function<Ret(Args const&...)> expression) :
	definition<Ret(Args...)>(),
  m_evaluate(expression)
{
  m_evaluate = expression;
}

template <typename Ret, typename... Args>
Ret ana::column::equation<Ret(Args...)>::evaluate(ana::observable<Args>... args) const
{
  return this->m_evaluate(args.value()...);
}

template <typename Ret, typename... Args>
auto ana::make_equation(std::function<Ret(Args...)> func) -> std::shared_ptr<ana::equation_t<std::function<Ret(Args...)>>>
{
	auto eqn = std::make_shared<column::equation<std::decay_t<Ret>(std::decay_t<Args>...)>>(func);
	return eqn;
}