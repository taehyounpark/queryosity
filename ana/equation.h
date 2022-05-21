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
class column<Ret>::equation : public column<Ret>::calculation
{

public:
  using argtuple_type = std::tuple<std::shared_ptr<cell<Args>>...>;
  using evalfunc_type = std::function<Ret(const Args&...)>;

public:
  equation(const std::string& name);
	virtual ~equation() = default;

  template <typename F>
	void set_evaluation(F&& callable);

  // convert each input argument type
  template <typename... UArgs>
  void input_arguments(cell<UArgs>&... args);

  virtual Ret calculate() const override;

  auto get_arguments() const -> argtuple_type;
  std::vector<std::string> get_argument_names() const;

protected:
	argtuple_type m_arguments;
	evalfunc_type m_evalute;

};

template <typename Ret, typename... Args>
std::shared_ptr<typename ana::column<std::decay_t<Ret>>::template equation<std::decay_t<Args>...>> make_equation(const std::string& name, std::function<Ret(Args...)> func)
{
	(void)(func);
	auto eqn = std::make_shared<typename ana::column<std::decay_t<Ret>>::template equation<std::decay_t<Args>...>>(name);
	return eqn;
}

}

template <typename Ret>
template <typename... Args>
ana::column<Ret>::equation<Args...>::equation(const std::string& name) :
	column<Ret>::calculation(name)
{}

template <typename Ret>
template <typename... Args>
template <typename F>
void ana::column<Ret>::equation<Args...>::set_evaluation(F&& callable)
{
	m_evalute = std::function<Ret(const Args&...)>(std::forward<F>(callable));
}

template <typename Ret>
template <typename... Args>
template <typename... UArgs>
void ana::column<Ret>::equation<Args...>::input_arguments(cell<UArgs>&... args)
{
  static_assert(sizeof...(Args)==sizeof...(UArgs));
  m_arguments = std::make_tuple(
    std::invoke(
      [](cell<UArgs>& args) -> std::shared_ptr<cell<Args>> {
        return ana::value_as<Args>(args);
    },args)...
  );
}

template <typename Ret>
template <typename... Args>
auto ana::column<Ret>::equation<Args...>::get_arguments() const -> argtuple_type
{
  return m_arguments;
}

template <typename Ret>
template <typename... Args>
std::vector<std::string> ana::column<Ret>::equation<Args...>::get_argument_names() const
{
  std::vector<std::string> argument_namess;
  std::apply([&argument_namess](const std::shared_ptr<cell<Args>>&... args) {
    (argument_namess.push_back(args->name()),...);
  },m_arguments);
  return argument_namess;
}

// user-defined evaluation with input arguments
template <typename Ret>
template <typename... Args>
Ret ana::column<Ret>::equation<Args...>::calculate() const
{
  return std::apply(
    [this](const std::shared_ptr<cell<Args>>&... args) { 
      return this->m_evalute(args->value()...);
    },m_arguments
  );
}