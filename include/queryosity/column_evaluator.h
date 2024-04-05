#pragma once

#include <functional>
#include <memory>
#include <type_traits>

#include "action.h"
#include "column_customization.h"

namespace queryosity {

namespace column {

template <typename T> class evaluator {

public:
  using evaluated_type = T;

public:
  template <typename... Args> evaluator(Args const &...args);
  virtual ~evaluator() = default;

  void patch(std::function<void(T *)> fn);

  template <typename... Vals>
  std::unique_ptr<T> evaluate(view<Vals> const &...cols) const;

protected:
  std::function<std::unique_ptr<T>()> m_make;
  std::function<void(T *)> m_patch;
};
} // namespace column

} // namespace queryosity

template <typename T>
template <typename... Args>
queryosity::column::evaluator<T>::evaluator(Args const &...args)
    : m_make(std::bind(
          [](Args const &...args) { return std::make_unique<T>(args...); },
          args...)),
      m_patch([](T *) {}) {}

template <typename T>
void queryosity::column::evaluator<T>::patch(std::function<void(T *)> fn) {
  m_patch = std::move(fn);
}

template <typename T>
template <typename... Vals>
std::unique_ptr<T>
queryosity::column::evaluator<T>::evaluate(view<Vals> const &...columns) const {
  auto defn = m_make();
  this->m_patch(defn.get());
  defn->set_arguments(columns...);
  return defn;
}