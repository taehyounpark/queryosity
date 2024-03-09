#pragma once

#include <memory>
#include <tuple>

#include "column_calculation.h"
#include "column_evaluate.h"
#include "todo.h"

namespace queryosity {

class dataflow;

/**
 * @ingroup abc
 * @brief Column with user-defined return value type and evaluation
 * dataset.
 * @tparam Out Output data type.
 * @tparam Ins Input column data type(s).
 */
template <typename Out, typename... Ins>
class column::definition<Out(Ins...)> : public column::calculation<Out> {

public:
  using vartuple_type = std::tuple<variable<Ins>...>;
  using obstuple_type = std::tuple<observable<Ins>...>;

public:
  /**
   * @brief Default constructor.
   */
  definition() = default;
  /**
   * @brief Default destructor.
   */
  virtual ~definition() = default;

public:
  virtual Out calculate() const final override;

  /**
   * @brief Compute the quantity of interest for the entry
   * @note Columns passed in as observables are not computed until `value()` is
   * called.
   * @param args Input observables.
   */
  virtual Out evaluate(observable<Ins>... args) const = 0;

  template <typename... Args> void set_arguments(const view<Args> &...args);

protected:
  vartuple_type m_arguments;
};

/**
 * @ingroup api
 * @brief Define a custom column in dataflow.
 */
template <typename Def> class column::definition {

public:
  /**
   * @param args Constructor arguments of @p Def.
   * @brief Define a custom column in dataflow.
   */
  template <typename... Args> definition(Args const &...args);

  auto _define(dataflow &df) const;

protected:
  std::function<todo<evaluate_t<Def>>(dataflow &)> m_define;
};

} // namespace queryosity

#include "dataflow.h"

template <typename Out, typename... Ins>
template <typename... Args>
void queryosity::column::definition<Out(Ins...)>::set_arguments(
    view<Args> const &...args) {
  static_assert(sizeof...(Ins) == sizeof...(Args));
  m_arguments = std::make_tuple(std::invoke(
      [](const view<Args> &args) -> variable<Ins> {
        return variable<Ins>(args);
      },
      args)...);
}

template <typename Out, typename... Ins>
Out queryosity::column::definition<Out(Ins...)>::calculate() const {
  return std::apply(
      [this](const variable<Ins> &...args) { return this->evaluate(args...); },
      m_arguments);
}

template <typename Def>
template <typename... Args>
queryosity::column::definition<Def>::definition(Args const &...args) {
  m_define = [args...](dataflow &df) { return df._define<Def>(args...); };
}

template <typename Def>
auto queryosity::column::definition<Def>::_define(dataflow &df) const {
  return this->m_define(df);
}