#pragma once

#include <memory>

#include "dataset.h"

namespace queryosity {
namespace dataset {

template <typename DS> struct input {
  template <typename... Args> input(Args &&...args);
  virtual ~input() = default;
  std::unique_ptr<DS> ds;
};

} // namespace dataset

} // namespace queryosity

template <typename DS>
template <typename... Args>
queryosity::dataset::input<DS>::input(Args &&...args)
    : ds(std::make_unique<DS>(std::forward<Args>(args)...)) {}