#pragma once

#include <functional>
#include <memory>
#include <type_traits>

#include "action.h"

namespace ana {

template <typename T> class column::evaluate {

public:
  using evaluated_type = T;

public:
  template <typename... Args> evaluate(Args const &...args);
  ~evaluate() = default;

  template <typename... Vals>
  std::unique_ptr<T> _evaluate(cell<Vals> const &...cols) const;

protected:
  std::function<std::unique_ptr<T>()> m_make_unique;
};

} // namespace ana

template <typename T>
template <typename... Args>
ana::column::evaluate<T>::evaluate(Args const &...args)
    : m_make_unique(std::bind(
          [](Args const &...args) { return std::make_unique<T>(args...); },
          args...)) {}

template <typename T>
template <typename... Vals>
std::unique_ptr<T>
ana::column::evaluate<T>::_evaluate(cell<Vals> const &...columns) const {
  auto defn = m_make_unique();

  defn->set_arguments(columns...);

  return defn;
}