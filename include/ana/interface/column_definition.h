#pragma once

#include <memory>
#include <tuple>

#include "column_calculation.h"
#include "column_evaluator.h"
#include "todo.h"

namespace ana {

class dataflow;

/**
 * @brief Column with user-defined return value type and evaluation
 * dataset.
 * @tparam T return value type.
 */
template <typename Ret, typename... Vals>
class column::definition<Ret(Vals...)> : public column::calculation<Ret> {

public:
  using vartuple_type = std::tuple<variable<Vals>...>;
  using obstuple_type = std::tuple<observable<Vals>...>;

public:
  definition() = default;
  virtual ~definition() = default;

protected:
  template <typename... Args> definition(Args &&...args);

public:
  virtual Ret calculate() const final override;
  virtual Ret evaluate(observable<Vals>... args) const = 0;

  template <typename... Ins> void set_arguments(const cell<Ins> &...args);

protected:
  vartuple_type m_arguments;
};

template <typename Def> class column::definition {

public:
  template <typename... Args> definition(Args const &...args);

  auto _define(dataflow &df) const;

protected:
  std::function<todo<evaluator_t<Def>>(dataflow &)> m_define;
};

} // namespace ana

#include "dataflow.h"

template <typename Ret, typename... Vals>
template <typename... Args>
ana::column::definition<Ret(Vals...)>::definition(Args &&...args)
    : calculation<Ret>(std::forward<Args>(args)...) {}

template <typename Ret, typename... Vals>
template <typename... Ins>
void ana::column::definition<Ret(Vals...)>::set_arguments(
    cell<Ins> const &...args) {
  static_assert(sizeof...(Vals) == sizeof...(Ins));
  m_arguments = std::make_tuple(std::invoke(
      [](const cell<Ins> &args) -> variable<Vals> {
        return variable<Vals>(args);
      },
      args)...);
}

template <typename Ret, typename... Vals>
Ret ana::column::definition<Ret(Vals...)>::calculate() const {
  return std::apply(
      [this](const variable<Vals> &...args) { return this->evaluate(args...); },
      m_arguments);
}

template <typename Def>
template <typename... Args>
ana::column::definition<Def>::definition(Args const &...args) {
  m_define = [args...](dataflow &df) { return df._define<Def>(args...); };
}

template <typename Def>
auto ana::column::definition<Def>::_define(dataflow &df) const {
  return this->m_define(df);
}