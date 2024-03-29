#pragma once

#include "query.h"

namespace queryosity {

/**
 * @brief Minimal query with an output result.
 * @details This ABC should be used for actions that do not require any input
 * columns.
 */
template <typename T> class query::aggregation : public node {

public:
  using result_type = T;

public:
  aggregation() = default;
  virtual ~aggregation() = default;

  /**
   * Create and return the result of the query.
   * If multiple slots are run concurrently in multithreaded mode, the results
   * are merged into one (see `merge`).
   * @return The result.
   */
  virtual T result() const = 0;

  /**
   * Merge the results from concurrent slots into one representing the full
   * dataset.
   * @param[in] results Partial result from each thread.
   * @return Merged result.
   */
  virtual T merge(std::vector<T> const &results) const = 0;

  using node::count;

  /**
   * Shortcut for `result()`.
   * @return The result.
   */
  T operator->() const { return this->result(); }

protected:
};

} // namespace queryosity

#include "selection.h"