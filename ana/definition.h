#pragma once

#include <memory>
#include <tuple>
#include <functional>

#include "ana/column.h"
#include "ana/calculation.h"

namespace ana
{

//------------------------------------------------------------------------------
// definition: column<T>::calculation with input arguments <U,V,W,...>
//------------------------------------------------------------------------------
template <typename Ret>
template <typename... Args>
class column<Ret>::definition : public column<Ret>::calculation
{

public:
  using argtup_type = std::tuple<std::shared_ptr<cell<Args>>...>;

public:
  definition(const std::string& name);
  virtual ~definition() = default;

  template <typename... UArgs>
  void input_arguments(cell<UArgs>&... args);

  virtual Ret calculate() const override;
  virtual Ret evaluate(observable<Args>... args) const = 0;

  auto get_arguments() const -> argtup_type;
  std::vector<std::string> get_argument_names() const;

protected:
	argtup_type m_arguments;

};

}

template <typename Ret>
template <typename... Args>
ana::column<Ret>::definition<Args...>::definition(const std::string& name) :
  column<Ret>::calculation(name)
{}

template <typename Ret>
template <typename... Args>
template <typename... UArgs>
void ana::column<Ret>::definition<Args...>::input_arguments(cell<UArgs>&... args)
{
  static_assert(sizeof...(Args)==sizeof...(UArgs));
  m_arguments = std::make_tuple(
    std::invoke(
      [](const cell<UArgs>& args) -> std::shared_ptr<cell<Args>> {
        return ana::value_as<Args>(args);
    },args)...
  );
}

template <typename Ret>
template <typename... Args>
auto ana::column<Ret>::definition<Args...>::get_arguments() const -> argtup_type
{
  return m_arguments;
}

template <typename Ret>
template <typename... Args>
std::vector<std::string> ana::column<Ret>::definition<Args...>::get_argument_names() const
{
  std::vector<std::string> argNames;
  std::apply([&argNames](const std::shared_ptr<cell<Args>>&... args) {
    (argNames.push_back(args->name()),...);
  },m_arguments);
  return argNames;
}

// user-defined evaluation with input arguments
template <typename Ret>
template <typename... Args>
Ret ana::column<Ret>::definition<Args...>::calculate() const
{
  return std::apply(
    [this](const std::shared_ptr<cell<Args>>&... args) { 
      return this->evaluate(*args...);
    },m_arguments
  );
}