#pragma once

#include <array>
#include <string>
#include <tuple>
#include <utility>

#include "dataset.h"

namespace queryosity {

class dataflow;

/**
 * @ingroup api
 * @brief Argument for queryosity::dataflow::read().
 * @tparam Val Column data type.
 */
template <typename Val> class dataset::column {

public:
  /**
   * @brief Constructor.
   * @param[in] column_name Name of column.
   */
  column(const std::string &column_name);
  ~column() = default;

  template <typename DS> auto _read(dataset::loaded<DS> &ds) const;

protected:
  std::string m_name;
};

} // namespace queryosity

#include "dataflow.h"
#include "dataset_loaded.h"

template <typename Val>
queryosity::dataset::column<Val>::column(const std::string &name)
    : m_name(name) {}

template <typename Val>
template <typename DS>
auto queryosity::dataset::column<Val>::_read(
    queryosity::dataset::loaded<DS> &ds) const {
  return ds.template _read<Val>(this->m_name);
}