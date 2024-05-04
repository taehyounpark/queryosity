#pragma once

#include <functional>

#include "column_definition.hpp"

namespace queryosity {

namespace column {

template <typename Out, typename... Ins>
class equation<Out(Ins...)> : public definition<Out(Ins...)> {

public:
  using vartuple_type = typename definition<Out(Ins...)>::vartuple_type;
  using function_type =
      std::function<std::decay_t<Out>(std::decay_t<Ins> const &...)>;

public:
  template <typename Fn> equation(Fn&& fn);
  virtual ~equation() = default;

public:
  virtual Out evaluate(observable<Ins>... args) const final override;

  virtual void initialize(unsigned int slot, unsigned long long begin,
                          unsigned long long end) final override;
  virtual void execute(unsigned int slot, unsigned long long entry) final override;
  virtual void finalize(unsigned int slot) final override;

protected:
  function_type m_evaluate;
};

} // namespace column

} // namespace queryosity

template <typename Out, typename... Ins>
template <typename Fn>
queryosity::column::equation<Out(Ins...)>::equation(Fn&& fn) : m_evaluate(std::forward<Fn>(fn)) {}

template <typename Out, typename... Ins>
Out queryosity::column::equation<Out(Ins...)>::evaluate(
    observable<Ins>... args) const {
  return this->m_evaluate(args.value()...);
}

template <typename Out, typename... Ins>
void queryosity::column::equation<Out(Ins...)>::initialize(unsigned int slot, unsigned long long begin,
                                               unsigned long long end) {
  calculation<Out>::initialize(slot, begin, end);
                                               }

template <typename Out, typename... Ins>
void queryosity::column::equation<Out(Ins...)>::execute(unsigned int slot, unsigned long long entry) {
  calculation<Out>::execute(slot, entry);
}

template <typename Out, typename... Ins>
void queryosity::column::equation<Out(Ins...)>::finalize(unsigned int slot) {
  calculation<Out>::finalize(slot);
}