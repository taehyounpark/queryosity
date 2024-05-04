#pragma once

#include <memory>

#include "dataset.hpp"

namespace queryosity {

namespace dataset {

/**
 * @ingroup api
 * @brief Argument to load a dataset input in the dataflow.
 * @tparam DS Concrete implementation of `queryosity::dataset::reader`.
 */
template <typename DS> struct input {
  /**
   * @brief Argument constructor.
   * @tparam Args Constructor arguments types for @p DS.
   * @param[in] args Constructor arguments for @p DS.
   */
  input() = default;
  template <typename... Args> input(Args &&...args);
  virtual ~input() = default;
  std::unique_ptr<DS> ds; //!
};

} // namespace dataset

} // namespace queryosity

template <typename DS>
template <typename... Args>
queryosity::dataset::input<DS>::input(Args &&...args)
    : ds(std::make_unique<DS>(std::forward<Args>(args)...)) {}