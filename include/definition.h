#pragma once

#include <memory>
#include <tuple>
#include <functional>

#include "ana/column.h"
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
  defined_from();
  virtual ~defined_from() = default;

  template <typename... Vals>
  void set_arguments(const cell<Vals>&... args);

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
  definition();
  virtual ~definition() = default;

};

}

template <typename Ret>
template <typename... Args>
ana::term<Ret>::defined_from<Args...>::defined_from() :
  term<Ret>::calculation()
{}

template <typename Ret, typename... Args>
ana::column::definition<Ret(Args...)>::definition() :
  term<Ret>::template defined_from<Args...>()
{}

template <typename Ret>
template <typename... Args>
template <typename... Vals>
void ana::term<Ret>::defined_from<Args...>::set_arguments(cell<Vals> const&... args)
{
  static_assert(sizeof...(Args)==sizeof...(Vals));
  m_arguments = std::make_tuple(
    std::invoke(
      [](const cell<Vals>& args) -> variable<Args> {
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

// user-defined expression with input arguments
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