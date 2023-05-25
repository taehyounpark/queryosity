// #pragma once

// #include "ana/column.h"
// #include "ana/definition.h"

// namespace ana
// {

// template <typename Agg, typename... Vals>
// class column::aggregate<Agg(Vals...)> : public column::definition<Agg(Vals...)>
// {

// public:
//   using vartuple_type = typename definition<Agg(Vals...)>::vartuple_type;
//   using obstuple_type = typename definition<Agg(Vals...)>::obstuple_type;

// public:
//   aggregate() = default;
//   virtual ~aggregate() = default;

//   virtual Agg evaluate(ana::observable<Vals>... observables) const override
//   {
//     auto agg = Agg();
//     agg.set_observables(observables...);
//     return agg;
//   }

//   template <unsigned int N>
//   auto value() -> decltype(std::get<N>(std::declval<obstuple_type>()).value());

// protected:
//   obstuple_type m_observables;

// };

// }

// template <typename Agg>
// template <typename... Vals>
// void ana::column::aggregate<Agg(Vals...)>::set_observables(observable<Vals> const&... observables)
// {
//   m_observables = std::make_tuple(observables...);
// }


// template <typename Agg>
// template <typename... Vals>
// template <unsigned int N>
// auto ana::column::aggregate<Agg(Vals...)>::value() -> decltype(std::get<N>(std::declval<obstuple_type>()).value())
// {
//   return std::get<N>(m_observables).value();
// }

#pragma once

#include <memory>
#include <tuple>
#include <functional>

#include "ana/column.h"
#include "ana/calculation.h"

namespace ana
{

//------------------------------------------------------------------------------
// aggregated_from: term<T>::calculation with input arguments <U,V,W,...>
//------------------------------------------------------------------------------
template <typename Ret>
template <typename... Args>
class term<Ret>::aggregated_from : public term<Ret>
{

public:
  using vartuple_type = std::tuple<variable<Args>...>;
  using obstuple_type = std::tuple<observable<Args>...>;

public:
  aggregated_from() = default;
  virtual ~aggregated_from() = default;

  template <typename... Vals>
  void set_components(const cell<Vals>&... args);

  virtual void execute() override;
  virtual void aggregate(ana::observable<Args>... observables);
  virtual Ret const& value() const override;

protected:
	vartuple_type m_arguments;
	obstuple_type m_observables;

};

template <typename Ret, typename... Args>
class column::definition<Ret(Args...)> : public term<Ret>::template aggregated_from<Args...>
{

public:
  using vartuple_type = typename term<Ret>::template aggregated_from<Ret(Args...)>::vartuple_type;
  using obstuple_type = typename term<Ret>::template aggregated_from<Ret(Args...)>::obstuple_type;

public:
  definition();
  virtual ~definition() = default;

};

template <typename T, typename = void> struct column_evaluator_traits;
template <typename T> struct column_evaluator_traits<T, typename std::enable_if_t<ana::is_column_definition_v<T>>> { using evaluator_type = typename ana::column::template evaluator<T>; };

}

template <typename Ret, typename... Args>
ana::column::definition<Ret(Args...)>::definition() :
  term<Ret>::template aggregated_from<Args...>()
{}

template <typename Ret>
template <typename... Args>
template <typename... Vals>
void ana::term<Ret>::aggregated_from<Args...>::set_components(cell<Vals> const&... args)
{
  static_assert(sizeof...(Args)==sizeof...(Vals));
  m_arguments = std::make_tuple(
    std::invoke(
      [](const cell<Vals>& args) -> variable<Args> {
        return variable<Args>(args);
    },args)...
  );
}

// user-defined expression with input arguments
template <typename Ret>
template <typename... Args>
Ret ana::term<Ret>::aggregated_from<Args...>::aggregate() const
{
  return std::apply(
    [this](const variable<Args>&... args) { 
      return this->evaluate(args...);
    },m_arguments
  );
}