#pragma once

#include <memory>

#include "dataset.h"

namespace queryosity {

namespace dataset {

/**
 * @ingroup api
 * @brief Dataset input argument for dataflow.
 * @tparam DS queryosity::dataset::reader implementation.
 * @tparam Args Constructor arguments for `DS`.
 */
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