#pragma once

#include <string>
#include <memory>
#include <functional>

#include "ana/column.h"
#include "ana/calculation.h"

namespace ana
{

template <typename Ret>
template <typename... Args>
class term<Ret>::evaluated_from : public term<Ret>::calculation
{

public:
  using argtuple_type = std::tuple<std::shared_ptr<cell<Args>>...>;
  using evalfunc_type = std::function<Ret(const Args&...)>;

public:
  template <typename F>
  evaluated_from(F&& callable);
	virtual ~evaluated_from() = default;

  // template <typename F>
	// void set_expression(F&& callable);

  // convert each input argument type
  template <typename... UArgs>
  void set_arguments(cell<UArgs> const&... args);

  virtual Ret calculate() const override;

  auto get_arguments() const -> argtuple_type;

protected:
	argtuple_type m_arguments;
	evalfunc_type m_evaluate;

};

template <typename Ret, typename... Args>
class column::equation<Ret(Args...)> : public term<Ret>::template evaluated_from<Args...>
{
public:
  template <typename F>
  equation(F&& callable);
	virtual ~equation() = default;
};

template <typename Ret, typename... Args>
auto make_equation(std::function<Ret(Args...)> func) -> std::shared_ptr<column::equation<std::decay_t<Ret>(std::decay_t<Args>...)>>;

template <typename F> using equation_t = typename decltype(make_equation(std::function(std::declval<F>())))::element_type;

}

template <typename Ret>
template <typename... Args>
template <typename F>
ana::term<Ret>::evaluated_from<Args...>::evaluated_from(F&& callable) :
	term<Ret>::calculation()
{
  m_evaluate = std::function<Ret(const Args&...)>(std::forward<F>(callable));
}

template <typename Ret, typename... Args>
template <typename F>
ana::column::equation<Ret(Args...)>::equation(F&& callable) :
	term<Ret>::template evaluated_from<Args...>(std::forward<F>(callable))
{}

template <typename Ret>
template <typename... Args>
template <typename... UArgs>
void ana::term<Ret>::evaluated_from<Args...>::set_arguments(cell<UArgs> const&... args)
{
  static_assert(sizeof...(Args)==sizeof...(UArgs));
  m_arguments = std::make_tuple(
    std::invoke(
      [](cell<UArgs> const& args) -> std::shared_ptr<cell<Args>> {
        return ana::cell_as<Args>(args);
    },args)...
  );
}

template <typename Ret>
template <typename... Args>
auto ana::term<Ret>::evaluated_from<Args...>::get_arguments() const -> argtuple_type
{
  return m_arguments;
}

// user-defined expression with input arguments
template <typename Ret>
template <typename... Args>
Ret ana::term<Ret>::evaluated_from<Args...>::calculate() const
{
  return std::apply(
    [this](const std::shared_ptr<cell<Args>>&... args) { 
      return this->m_evaluate(args->value()...);
    },m_arguments
  );
}

template <typename Ret, typename... Args>
auto ana::make_equation(std::function<Ret(Args...)> func) -> std::shared_ptr<ana::column::equation<std::decay_t<Ret>(std::decay_t<Args>...)>>
{
	auto eqn = std::make_shared<column::equation<std::decay_t<Ret>(std::decay_t<Args>...)>>(func);
	return eqn;
}