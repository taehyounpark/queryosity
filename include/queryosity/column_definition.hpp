#pragma once

#include <memory>
#include <tuple>

#include "column_calculation.hpp"
#include "column_evaluator.hpp"
#include "todo.hpp"

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
  definition() = default;
  virtual ~definition() = default;

public:
  virtual Out calculate() const final override;

  /**
   * @brief Compute the quantity of interest for the entry
   * @note Columns observables are not computed until `value()` is
   * called.
   * @param[in] args Input column observables.
   */
  virtual Out evaluate(observable<Ins>... args) const = 0;

  template <typename... Args> void set_arguments(const view<Args> &...args);

protected:
  vartuple_type m_arguments;
};

/**
 * @ingroup api
 * @brief Argument to define a custom column in the dataflow.
 * @tparam Def Concrete implementation of
 * `queryosity::column::definition<Out(Ins...)>`
 */
template <typename Def> class column::definition {

public:
  /**
   * @brief Argument constructor.
   * @param[in] args Constructor arguments of @p Def.
   */
  template <typename... Args> definition(Args const &...args);

  auto _define(computation &comp) const;

  template <typename Sel> auto _select(selection::cutflow &cflw) const;

  template <typename Sel>
  auto _select(selection::cutflow &cflw, selection::node const &presel) const;

protected:
  std::function<std::unique_ptr<evaluator<Def>>(computation &)> m_define;
};

} // namespace queryosity

#include "dataflow.hpp"
#include "selection_applicator.hpp"

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
  m_define = [args...](computation &comp) { return comp.define<Def>(args...); };
}

template <typename Def>
auto queryosity::column::definition<Def>::_define(
    queryosity::column::computation &comp) const {
  return this->m_define(comp);
}

template <typename Def>
template <typename Sel>
auto queryosity::column::definition<Def>::_select(
    selection::cutflow &cflw) const {
  return cflw.template select<Sel>(nullptr, this->m_define(cflw));
}

template <typename Def>
template <typename Sel>
auto queryosity::column::definition<Def>::_select(
    selection::cutflow &cflw, selection::node const &presel) const {
  return cflw.template select<Sel>(&presel, this->m_define(cflw));
}