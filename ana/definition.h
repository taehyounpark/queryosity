#pragma once

#include <memory>
#include <tuple>
#include <functional>

#include "ana/column.h"
#include "ana/calculation.h"

namespace ana
{

//------------------------------------------------------------------------------
// calculated_with: term<T>::calculation with input arguments <U,V,W,...>
//------------------------------------------------------------------------------
template <typename Ret>
template <typename... Args>
class term<Ret>::calculated_with : public term<Ret>::calculation
{

public:
  using vartuple_type = std::tuple<variable<Args>...>;
  using obstuple_type = std::tuple<observable<Args>...>;

public:
  calculated_with() = default;
  virtual ~calculated_with() = default;

  template <typename... Vals>
  void set_arguments(const cell<Vals>&... args);

  virtual Ret calculate() const override;
  virtual Ret evaluate(observable<Args>... args) const = 0;

  auto get_arguments() const -> vartuple_type;

protected:
	vartuple_type m_arguments;

};

template <typename Ret, typename... Args>
class column::definition<Ret(Args...)> : public term<Ret>::template calculated_with<Args...>
{

public:
  using vartuple_type = typename term<Ret>::template calculated_with<Ret(Args...)>::vartuple_type;
  using obstuple_type = typename term<Ret>::template calculated_with<Ret(Args...)>::obstuple_type;

public:
  definition();
  virtual ~definition() = default;

};

template <typename T, typename = void> struct column_evaluator_traits;
template <typename T> struct column_evaluator_traits<T, typename std::enable_if_t<ana::is_column_definition_v<T>>> { using evaluator_type = typename ana::column::template evaluator<T>; };

}

template <typename Ret, typename... Args>
ana::column::definition<Ret(Args...)>::definition() :
  term<Ret>::template calculated_with<Args...>()
{}

template <typename Ret>
template <typename... Args>
template <typename... Vals>
void ana::term<Ret>::calculated_with<Args...>::set_arguments(cell<Vals> const&... args)
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
auto ana::term<Ret>::calculated_with<Args...>::get_arguments() const -> vartuple_type
{
  return m_arguments;
}

// user-defined expression with input arguments
template <typename Ret>
template <typename... Args>
Ret ana::term<Ret>::calculated_with<Args...>::calculate() const
{
  return std::apply(
    [this](const variable<Args>&... args) { 
      return this->evaluate(args...);
    },m_arguments
  );
}