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
  using argtuple_type = typename definition<Ret(Args...)>::argtuple_type;
  using function_type = std::function<std::decay_t<Ret>(std::decay_t<Args> const&...)>;

public:
  template <typename F>
  equation(F callable);
	virtual ~equation() = default;

  virtual Ret evaluate(observable<Args>... args) const override;

protected:
	argtuple_type m_arguments;
	function_type m_evaluate;

};

template <typename F>
struct equation_traits { using equation_type = typename equation_traits<decltype(std::function{std::declval<F>()})>::equation_type; };
template <typename Ret, typename... Args>
struct equation_traits<std::function<Ret(Args...)>> { using equation_type = typename column::equation<std::decay_t<Ret>(std::decay_t<Args>...)>; };

template <typename F>
using equation_t = typename equation_traits<F>::equation_type;

template <typename F>
auto make_equation(F expression) -> std::shared_ptr<equation_t<F>>;

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

template <typename F> struct column_evaluator_traits<F, typename std::enable_if_t<!ana::is_column_definition_v<F> && ana::is_callable_v<F>>> { using evaluator_type = typename ana::column::template evaluator<ana::equation_t<F>>; };
template <typename T> using column_evaluator_t = typename column_evaluator_traits<T>::evaluator_type;

}

template <typename Ret, typename... Args>
template <typename F>
ana::column::equation<Ret(Args...)>::equation(F callable) :
  m_evaluate(callable)
{}

template <typename Ret, typename... Args>
Ret ana::column::equation<Ret(Args...)>::evaluate(ana::observable<Args>... args) const
{
  return this->m_evaluate(args.value()...);
}

template <typename F>
auto ana::make_equation(F expression) -> std::shared_ptr<ana::equation_t<F>>
{
	return std::make_shared<ana::equation_t<F>>(expression);
}