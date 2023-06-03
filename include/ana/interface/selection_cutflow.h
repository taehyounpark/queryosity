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
  auto filter(const std::string &name, F expression) const
      -> std::unique_ptr<applicator<column::template equation_t<F>>>;

  template <typename Sel, typename F>
  auto channel(const std::string &name, F expression) const
      -> std::unique_ptr<applicator<column::template equation_t<F>>>;

  template <typename Sel, typename F>
  auto filter(selection const &prev, const std::string &name,
              F expression) const
      -> std::unique_ptr<applicator<column::template equation_t<F>>>;

  template <typename Sel, typename F>
  auto channel(selection const &prev, const std::string &name,
               F expression) const
      -> std::unique_ptr<applicator<column::template equation_t<F>>>;

  template <typename Sel, typename... Cols>
  auto apply_selection(applicator<Sel> const &calc, Cols const &...columns)
      -> std::unique_ptr<selection>;

  template <typename Sel>
  auto join(selection const &a, selection const &b) const
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
auto ana::selection::cutflow::filter(const std::string &name,
                                     F expression) const
    -> std::unique_ptr<applicator<column::template equation_t<F>>> {
  auto calc = std::make_unique<applicator<column::template equation_t<F>>>(
      std::function{expression});
  calc->template set_selection<Sel>(nullptr, false, name);
  return std::move(calc);
}

template <typename Sel, typename F>
auto ana::selection::cutflow::channel(const std::string &name,
                                      F expression) const
    -> std::unique_ptr<applicator<column::template equation_t<F>>> {
  auto calc = std::make_unique<applicator<column::template equation_t<F>>>(
      std::function{expression});
  calc->template set_selection<Sel>(nullptr, true, name);
  return std::move(calc);
}

template <typename Sel, typename F>
auto ana::selection::cutflow::filter(selection const &prev,
                                     const std::string &name,
                                     F expression) const
    -> std::unique_ptr<applicator<column::template equation_t<F>>> {
  auto calc = std::make_unique<applicator<column::template equation_t<F>>>(
      std::function{expression});
  calc->template set_selection<Sel>(&prev, false, name);
  return std::move(calc);
}

template <typename Sel, typename F>
auto ana::selection::cutflow::channel(selection const &prev,
                                      const std::string &name,
                                      F expression) const
    -> std::unique_ptr<applicator<column::template equation_t<F>>> {
  auto calc = std::make_unique<applicator<column::template equation_t<F>>>(
      std::function{expression});
  calc->template set_selection<Sel>(&prev, true, name);
  return std::move(calc);
}

template <typename Sel, typename... Cols>
auto ana::selection::cutflow::apply_selection(applicator<Sel> const &calc,
                                              Cols const &...columns)
    -> std::unique_ptr<selection> {
  auto sel = calc.apply_selection(columns...);
  this->add_selection(*sel);
  return std::move(sel);
}

template <typename Sel>
auto ana::selection::cutflow::join(ana::selection const &a,
                                   ana::selection const &b) const
    -> std::unique_ptr<ana::selection> {
  return std::make_unique<Sel>(a, b);
}

inline void ana::selection::cutflow::add_selection(ana::selection &sel) {
  m_selections.push_back(&sel);
}