#pragma once

#include <array>
#include <string>
#include <tuple>
#include <utility>

#include "dataset.hpp"

namespace queryosity {

class dataflow;

/**
 * @ingroup api
 * @brief Argument to read a column from a loaded dataset.
 * @tparam Val Column data type.
 */
template <typename Val> struct dataset::column {

public:
  /**
   * @brief Argument constructor.
   * @param[in] column_name Name of column.
   */
  column(const std::string &column_name);
  ~column() = default;

  template <typename DS> auto _read(dataset::loaded<DS> &ds) const;

protected:
  std::string m_name;
};

} // namespace queryosity

#include "dataflow.hpp"
#include "dataset_loaded.hpp"

template <typename Val>
queryosity::dataset::column<Val>::column(const std::string &name)
    : m_name(name) {}

template <typename Val>
template <typename DS>
auto queryosity::dataset::column<Val>::_read(
    queryosity::dataset::loaded<DS> &ds) const {
  return ds.template _read<Val>(this->m_name);
}