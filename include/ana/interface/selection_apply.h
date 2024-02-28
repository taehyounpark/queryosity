#pragma once

#include <memory>
#include <string>
#include <vector>

#include "selection.h"

namespace ana {

template <typename T> class ana::selection::apply {

public:
  template <typename Fn> apply(Fn fn);
  ~apply() = default;

  template <typename Sel> void set_selection(const selection::node *presel);

  template <typename... Vals>
  std::unique_ptr<selection::node> _apply(cell<Vals> const &...columns) const;

protected:
  std::function<std::unique_ptr<T>()> m_make_unique_equation;
  std::function<std::unique_ptr<selection::node>()> m_make_unique_selection;
};

} // namespace ana

#include "column_equation.h"

template <typename T>
template <typename Fn>
ana::selection::apply<T>::apply(Fn fn)
    : m_make_unique_equation([fn]() { return std::make_unique<T>(fn); }),
      m_make_unique_selection([]() { return nullptr; }) {}

template <typename T>
template <typename Sel>
void ana::selection::apply<T>::set_selection(const selection::node *presel) {
  m_make_unique_selection = [presel]() {
    return std::make_unique<Sel>(presel);
  };
}

template <typename T>
template <typename... Vals>
std::unique_ptr<ana::selection::node>
ana::selection::apply<T>::_apply(cell<Vals> const &...columns) const {

  // make selection
  auto sel = this->m_make_unique_selection();

  // set decision
  auto eqn = this->m_make_unique_equation();
  eqn->set_arguments(columns...);
  sel->set_decision(std::move(eqn));

  return sel;
}