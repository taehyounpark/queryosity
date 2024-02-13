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
  template <typename Sel, typename F>
  auto select(selection const *prev, F expression) const
      -> std::unique_ptr<applicator<column::template equation_t<F>>>;

  template <typename Sel, typename... Cols>
  auto _apply(applicator<Sel> const &calc, Cols const &...columns)
      -> std::unique_ptr<selection>;

protected:
  void add_selection(selection &selection);

protected:
  std::vector<selection *> m_selections;
};

} // namespace ana

#include "column.h"
#include "selection_applicator.h"
#include "selection_cut.h"
#include "selection_weight.h"

template <typename Sel, typename F>
auto ana::selection::cutflow::select(selection const *prev, F expression) const
    -> std::unique_ptr<applicator<column::template equation_t<F>>> {
  auto calc = std::make_unique<applicator<column::template equation_t<F>>>(
      std::function{expression});
  calc->template set_selection<Sel>(prev);
  return calc;
}

template <typename Sel, typename... Cols>
auto ana::selection::cutflow::_apply(applicator<Sel> const &calc,
                                     Cols const &...columns)
    -> std::unique_ptr<selection> {
  auto sel = calc._apply(columns...);
  this->add_selection(*sel);
  return sel;
}

inline void ana::selection::cutflow::add_selection(ana::selection &sel) {
  m_selections.push_back(&sel);
}