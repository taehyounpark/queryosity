#pragma once

#include <memory>
#include <tuple>
#include <functional>

#include "ana/column.h"
#include "ana/calculation.h"

namespace ana
{

//------------------------------------------------------------------------------
// aggregated_with: term<T>::calculation with input arguments <U,V,W,...>
//------------------------------------------------------------------------------
template <typename Ret>
template <typename... Cmps>
class term<Ret>::aggregated_with : public term<Ret>
{

public:
  using vartuple_type = std::tuple<variable<Cmps>...>;
  using obstuple_type = std::tuple<observable<Cmps>...>;

public:
  aggregated_with() = default;
  virtual ~aggregated_with() = default;

  template <typename... Vals>
  void set_components(cell<Vals> const&... args);

  virtual Ret const& value() const override;
  template <unsigned int N>
  auto value() -> decltype(std::get<N>(std::declval<vartuple_type>()).value());

protected:
	vartuple_type m_observables;

};

template <typename Ret, typename... Cmps>
class column::aggregate<Ret(Cmps...)> : public term<Ret>::template aggregated_with<Cmps...>
{

public:
  template <typenane Enum>
  class proxy;

public:
  using vartuple_type = typename term<Ret>::template aggregated_with<Ret(Cmps...)>::vartuple_type;
  using obstuple_type = typename term<Ret>::template aggregated_with<Ret(Cmps...)>::obstuple_type;

public:
  aggregate() = default;
  virtual ~aggregate() = default;

};

}

template <typename Ret>
template <typename... Cmps>
Ret const& ana::term<Ret>::aggregated_with<Cmps...>::value() const
{
  return *this;
}

template <typename Ret>
template <typename... Cmps>
template <typename... Vals>
void ana::term<Ret>::aggregated_with<Cmps...>::set_components(ana::cell<Vals> const&... args)
{
  static_assert(sizeof...(Cmps)==sizeof...(Vals));
  m_observables = std::make_tuple(
    std::invoke(
      [](const cell<Vals>& args) -> variable<Cmps> {
        return variable<Cmps>(args);
    },args)...
  );
}

template <typename Ret>
template <typename... Cmps>
template <unsigned int N>
auto ana::term<Ret>::aggregated_with<Cmps...>::value() -> decltype(std::get<N>(std::declval<vartuple_type>()).value())
{
  return std::get<N>(m_observables).value();
}