#pragma once

#include <functional>
#include <memory>
#include <type_traits>

#include "column_evaluator.hpp"

namespace queryosity {

template <typename Sel, typename Def>
class selection::applicator : public column::evaluator<Def> {

public:
  using selection_type = Sel;
  using evaluated_type = typename column::evaluator<Def>::evaluated_type;

public:
  template <typename... Args>
  applicator(selection::node const *prev, Args const &...args);
  virtual ~applicator() = default;

  template <typename... Vals>
  std::pair<std::unique_ptr<Sel>, std::unique_ptr<Def>>
  apply(column::view<Vals> const &...columns) const;

protected:
  selection::node const *m_prev;
};

} // namespace queryosity

template <typename Sel, typename Def>
template <typename... Args>
queryosity::selection::applicator<Sel, Def>::applicator(
    selection::node const *prev, Args const &...args)
    : column::evaluator<Def>(args...), m_prev(prev) {}

template <typename Sel, typename Def>
template <typename... Vals>
std::pair<std::unique_ptr<Sel>, std::unique_ptr<Def>>
queryosity::selection::applicator<Sel, Def>::apply(
    column::view<Vals> const &...columns) const {
  auto col = this->evaluate(columns...);
  auto sel = std::make_unique<Sel>(m_prev, column::variable<double>(*col));
  return {std::move(sel), std::move(col)};
}