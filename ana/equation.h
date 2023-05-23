#pragma once

#include <memory>
#include <tuple>
#include <functional>

#include "ana/definition.h"

namespace ana
{

template <typename Ret, typename... Args>
class column::equation<Ret(Args...)> : public column::definition<Ret(Args...)>
{

public:
  using argtuple_type = typename definition<Ret(Args...)>::argtup_type;
  using evalfunc_type = std::function<std::decay_t<Ret>(std::decay_t<Args> const&...)>;

public:
  equation(std::function<Ret(Args const&...)>);
	virtual ~equation() = default;

  virtual Ret evaluate(observable<Args>... args) const override;

protected:
	argtuple_type m_arguments;
	evalfunc_type m_evaluate;

};

template <typename T>
struct equation_trait_impl {};
template <typename Ret, typename... Args>
struct equation_trait_impl<std::function<Ret(Args...)>> { using equation_type = column::equation<std::decay_t<Ret>(std::decay_t<Args>...)>; };

template <typename F>
struct equation_trait { using equation_type = typename equation_trait_impl<decltype(std::function{std::declval<F>()})>::equation_type; };
template <typename Ret, typename... Args>
struct equation_trait<std::function<Ret(Args...)>> { using equation_type = typename equation_trait_impl<std::function<Ret(Args...)>>::equation_type; };

template <typename F>
using equation_t = typename equation_trait<F>::equation_type;

template <typename Ret, typename... Args>
auto make_equation(std::function<Ret(Args...)> func) -> std::shared_ptr<equation_t<std::function<Ret(Args...)>>>;

template <typename T>
struct is_callable
{
  typedef char yes;
  typedef long no;
  template <typename C> static yes check_callable( decltype(&C::operator()) ) ;
  template <typename C> static no check_callable(...);    
  enum { value = sizeof(check_callable<T>(0)) == sizeof(char) };
};
template <typename T>
constexpr bool is_callable_v = is_callable<T>::value;

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
	return std::make_shared<column::equation<std::decay_t<Ret>(std::decay_t<Args>...)>>(func);
}