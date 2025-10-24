#pragma once

#include <memory>
#include <string>
#include <vector>

#include "column_computation.hpp"
#include "selection.hpp"
#include "selection_applicator.hpp"

namespace queryosity {

class selection::cutflow : public column::computation {

public:
  cutflow() = default;
  virtual ~cutflow() = default;

public:
  template <typename Sel, typename Val>
  auto apply(selection::node const *prev, column::valued<Val> const &dec)
      -> selection::node *;

  template <typename Sel, typename Ret, typename... Args>
  auto select(selection::node const *prev, std::function<Ret(Args...)> fn) const
      -> std::unique_ptr<applicator<
          Sel, column::equation<std::decay_t<Ret>(std::decay_t<Args>...)>>>;

  template <typename Sel, typename Def>
  auto select(selection::node const *prev, std::unique_ptr<column::evaluator<Def>> eval) const
      -> std::unique_ptr<applicator<Sel, Def>>;

  template <typename Sel, typename Def, typename... Cols>
  auto apply(applicator<Sel, Def> const &calc, Cols const &...cols)
      -> selection::node *;

protected:
  template <typename Sel>
  auto add_selection(std::unique_ptr<Sel> selection) -> Sel *;

protected:
  std::vector<std::unique_ptr<selection::node>> m_selections;  //!
};

} // namespace queryosity

#include "column.hpp"
#include "selection_cut.hpp"
#include "selection_weight.hpp"

template <typename Sel, typename Val>
auto queryosity::selection::cutflow::apply(selection::node const *prev,
                                           column::valued<Val> const &dec)
    -> selection::node * {
  auto sel = std::make_unique<Sel>(prev, column::variable<double>(dec));
  return this->add_selection(std::move(sel));
}

template <typename Sel, typename Ret, typename... Args>
auto queryosity::selection::cutflow::select(
    selection::node const *prev, std::function<Ret(Args...)> fn) const
    -> std::unique_ptr<applicator<
        Sel, column::equation<std::decay_t<Ret>(std::decay_t<Args>...)>>> {
  return std::make_unique<applicator<
      Sel, column::equation<std::decay_t<Ret>(std::decay_t<Args>...)>>>(prev,
                                                                        fn);
}

template <typename Sel, typename Def>
auto queryosity::selection::cutflow::select(selection::node const *prev, std::unique_ptr<column::evaluator<Def>> eval) const
    -> std::unique_ptr<applicator<Sel, Def>> {
  return std::make_unique<applicator<Sel, Def>>(prev, std::move(*eval));
}

template <typename Sel, typename Def, typename... Cols>
auto queryosity::selection::cutflow::apply(
    selection::applicator<Sel, Def> const &calc, Cols const &...cols)
    -> selection::node * {
  auto [sel, col] = calc.apply(cols...);
  this->add_column(std::move(col));
  return this->add_selection(std::move(sel));
}

template <typename Sel>
auto queryosity::selection::cutflow::add_selection(std::unique_ptr<Sel> sel)
    -> Sel * {
  auto out = sel.get();
  m_selections.push_back(std::move(sel));
  return out;
}
