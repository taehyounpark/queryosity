#pragma once

#include <memory>
#include <string>
#include <vector>

#include "selection.h"

namespace ana {

class selection::cutflow {

public:
  cutflow() = default;
  ~cutflow() = default;

public:
  template <typename Sel, typename Fn>
  auto select(selection::node const *prev, Fn &&fn) const -> std::unique_ptr<
      selection::apply<column::template equation_t<std::decay_t<Fn>>>>;

  template <typename Sel, typename... Cols>
  auto _apply(apply<Sel> const &calc, Cols const &...columns)
      -> std::unique_ptr<selection::node>;

protected:
  void add_selection(selection::node &selection);

protected:
  std::vector<selection::node *> m_selections;
};

} // namespace ana

#include "column.h"
#include "selection_apply.h"
#include "selection_cut.h"
#include "selection_weight.h"

template <typename Sel, typename Fn>
auto ana::selection::cutflow::select(selection::node const *prev, Fn &&fn) const
    -> std::unique_ptr<
        selection::apply<column::template equation_t<std::decay_t<Fn>>>> {
  auto calc = std::make_unique<
      selection::apply<column::template equation_t<std::decay_t<Fn>>>>(
      std::forward<Fn>(fn));
  calc->template set_selection<Sel>(prev);
  return calc;
}

template <typename Sel, typename... Cols>
auto ana::selection::cutflow::_apply(apply<Sel> const &calc,
                                     Cols const &...columns)
    -> std::unique_ptr<selection::node> {
  auto sel = calc._apply(columns...);
  this->add_selection(*sel);
  return sel;
}

inline void ana::selection::cutflow::add_selection(ana::selection::node &sel) {
  m_selections.push_back(&sel);
}