#pragma once

#include <memory>
#include <string>
#include <vector>

#include "selection.h"

namespace ana {

template <typename T> class ana::selection::applicator {

public:
  template <typename Fn> applicator(Fn fn);
  ~applicator() = default;

  template <typename Sel> void set_selection(const selection *presel);

  template <typename... Vals>
  std::unique_ptr<selection> _apply(cell<Vals> const &...columns) const;

protected:
  std::function<std::unique_ptr<T>()> m_make_unique_equation;
  std::function<std::unique_ptr<selection>()> m_make_unique_selection;
};

} // namespace ana

#include "column_equation.h"

template <typename T>
template <typename Fn>
ana::selection::applicator<T>::applicator(Fn fn)
    : m_make_unique_equation([fn]() { return std::make_unique<T>(fn); }),
      m_make_unique_selection([]() { return nullptr; }) {}

template <typename T>
template <typename Sel>
void ana::selection::applicator<T>::set_selection(const selection *presel) {
  m_make_unique_selection = [presel]() {
    return std::make_unique<Sel>(presel);
  };
}

template <typename T>
template <typename... Vals>
std::unique_ptr<ana::selection>
ana::selection::applicator<T>::_apply(cell<Vals> const &...columns) const {

  // make selection
  auto sel = this->m_make_unique_selection();

  // set decision
  auto eqn = this->m_make_unique_equation();
  eqn->set_arguments(columns...);
  sel->set_decision(std::move(eqn));

  return sel;
}