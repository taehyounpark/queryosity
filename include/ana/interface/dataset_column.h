#pragma once

#include "dataset.h"

namespace ana {

class dataflow;

template <typename Val> class dataset::column {

public:
  column(const std::string &name);
  ~column() = default;

  template <typename DS> auto _read(dataset::opened<DS> &ds) const;

protected:
  std::string m_name;
};

} // namespace ana

#include "dataflow.h"
#include "dataset_opened.h"

template <typename Val>
ana::dataset::column<Val>::column(const std::string &name) : m_name(name) {}

template <typename Val>
template <typename DS>
auto ana::dataset::column<Val>::_read(ana::dataset::opened<DS> &ds) const {
  return ds.template _read<Val>(this->m_name);
}
