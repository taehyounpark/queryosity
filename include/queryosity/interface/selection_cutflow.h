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
  auto select(selection::node const *prev, column::cell<Val> const &dec)
      -> std::unique_ptr<selection::node>;

protected:
  void add_selection(selection::node &selection);

protected:
  std::vector<selection::node *> m_selections;
};

} // namespace queryosity

#include "column.h"
#include "selection_cut.h"
#include "selection_weight.h"

template <typename Sel, typename Val>
auto queryosity::selection::cutflow::select(selection::node const *prev,
                                            column::cell<Val> const &dec)
    -> std::unique_ptr<selection::node> {
  auto sel = std::make_unique<Sel>(prev, column::variable<double>(dec));
  this->add_selection(*sel);
  return sel;
}

inline void queryosity::selection::cutflow::add_selection(
    queryosity::selection::node &sel) {
  m_selections.push_back(&sel);
}