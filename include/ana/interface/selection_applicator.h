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
    : m_make_unique_equation(std::bind(
          [](Fn fn) -> std::unique_ptr<T> { return std::make_unique<T>(fn); },
          fn)),
      m_make_unique_selection(
          []() -> std::unique_ptr<selection> { return nullptr; }) {}

template <typename T>
template <typename Sel>
void ana::selection::applicator<T>::set_selection(const selection *presel) {
  m_make_unique_selection = std::bind(
      [](const selection *presel) -> std::unique_ptr<selection> {
        return std::make_unique<Sel>(presel);
      },
      presel);
}

template <typename T>
template <typename... Vals>
std::unique_ptr<ana::selection>
ana::selection::applicator<T>::_apply(cell<Vals> const &...columns) const {
  // make this selection
  auto eqn = this->m_make_unique_equation();
  eqn->set_arguments(columns...);

  auto sel = this->m_make_unique_selection();

  // set equation arguments

  // set selection decision
  // sel->set_decision(
  //     std::static_pointer_cast<term<cell_value_t<T>>>(m_equation));
  // auto eqn = std::unique_ptr<term<cell_value_t<T>>>(m_equation.release());
  sel->set_decision(std::move(eqn));

  return sel;
}