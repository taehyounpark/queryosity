#pragma once

#include <memory>
#include <string>
#include <vector>

#include "selection.h"

namespace queryosity {

class selection::cutflow {

public:
  cutflow() = default;
  ~cutflow() = default;

public:
  template <typename Sel, typename Val>
  auto select(selection::node const *prev, column::valued<Val> const &dec)
      -> selection::node *;

protected:
  template <typename Sel>
  auto add_selection(std::unique_ptr<Sel> selection) -> Sel *;

protected:
  std::vector<std::unique_ptr<selection::node>> m_selections;
};

} // namespace queryosity

#include "column.h"
#include "selection_cut.h"
#include "selection_weight.h"

template <typename Sel, typename Val>
auto queryosity::selection::cutflow::select(selection::node const *prev,
                                            column::valued<Val> const &dec)
    -> selection::node * {
  auto sel = std::make_unique<Sel>(prev, column::variable<double>(dec));
  return this->add_selection(std::move(sel));
}

template <typename Sel>
auto queryosity::selection::cutflow::add_selection(std::unique_ptr<Sel> sel)
    -> Sel * {
  auto out = sel.get();
  m_selections.push_back(std::move(sel));
  return out;
}