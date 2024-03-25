#pragma once

#include <memory>

#include "dataset.h"

namespace queryosity {

namespace dataset {

/**
 * @ingroup api
 * @brief Argument for queryosity::dataflow::load().
 * @tparam DS queryosity::dataset::reader implementation.
 */
template <typename DS> struct input {
  /**
   * @brief Constructor.
   * @tparam Args `DS` constructor argument types.
   * @param[in] args Constructor arguments for `DS`.
   */
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