#pragma once

#include <functional>

namespace queryosity {

namespace column {

template <typename Fn> struct customization {
  customization(Fn fn) : fn(std::move(fn)) {}
  ~customization() = default;
  template <typename Def> void patch(column::evaluator<Def> *eval) const;
  Fn fn;
};

} // namespace column

} // namespace queryosity

template <typename Fn>
template <typename Def>
void queryosity::column::customization<Fn>::patch(column::evaluator<Def> *eval) const {
  eval->patch(std::function<void(Def*)>(fn));
}