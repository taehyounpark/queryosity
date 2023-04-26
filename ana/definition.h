#pragma once

#include <memory>
#include <tuple>
#include <functional>

#include "ana/column.h"
#include "ana/term.h"
#include "ana/calculation.h"

namespace ana
{

//------------------------------------------------------------------------------
// defined_from: term<T>::calculation with input arguments <U,V,W,...>
//------------------------------------------------------------------------------
template <typename Ret>
template <typename... Args>
class term<Ret>::defined_from : public term<Ret>::calculation
{

public:
  using argtup_type = std::tuple<variable<Args>...>;

public:
  defined_from(const std::string& name);
  virtual ~defined_from() = default;

  template <typename... UArgs>
  void set_arguments(const cell<UArgs>&... args);

  virtual Ret calculate() const override;
  virtual Ret evaluate(observable<Args>... args) const = 0;

  auto get_arguments() const -> argtup_type;

protected:
	argtup_type m_arguments;

};

template <typename Ret, typename... Args>
class column::definition<Ret(Args...)> : public term<Ret>::template defined_from<Args...>
{

public:
  definition(const std::string& name);
  virtual ~definition() = default;

};

template <typename T>
struct is_column_definition : std::false_type {};
template <typename T>
struct is_column_definition<column::definition<T>> : std::true_type {};
template <typename T>
constexpr bool is_column_definition_v = is_column_definition<T>::value;

}

template <typename Ret>
template <typename... Args>
ana::term<Ret>::defined_from<Args...>::defined_from(const std::string& name) :
  term<Ret>::calculation(name)
{}

template <typename Ret, typename... Args>
ana::column::definition<Ret(Args...)>::definition(const std::string& name) :
  term<Ret>::template defined_from<Args...>(name)
{}

template <typename Ret>
template <typename... Args>
template <typename... UArgs>
void ana::term<Ret>::defined_from<Args...>::set_arguments(const cell<UArgs>&... args)
{
  static_assert(sizeof...(Args)==sizeof...(UArgs));
  m_arguments = std::make_tuple(
    std::invoke(
      [](const cell<UArgs>& args) -> variable<Args> {
        return variable<Args>(args);
    },args)...
  );
}

template <typename Ret>
template <typename... Args>
auto ana::term<Ret>::defined_from<Args...>::get_arguments() const -> argtup_type
{
  return m_arguments;
}

// user-defined evaluation with input arguments
template <typename Ret>
template <typename... Args>
Ret ana::term<Ret>::defined_from<Args...>::calculate() const
{
  return std::apply(
    [this](const variable<Args>&... args) { 
      return this->evaluate(args...);
    },m_arguments
  );
}