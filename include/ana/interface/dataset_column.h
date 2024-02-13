#pragma once

#include <array>
#include <string>
#include <tuple>
#include <utility>

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

template <typename... Vals> class dataset::columns {

public:
  template <typename... Names> columns(Names const &...names);
  columns(std::array<std::string, sizeof...(Vals)> const &names);
  ~columns() = default;

  template <std::size_t... Is> auto _read(std::index_sequence<Is...>) const {
    // call _read for each type and name, unpacking the indices
    return std::make_tuple(this->_read<Vals>(this->m_names[Is])...);
  }

  template <typename DS> auto _read(dataset::opened<DS> &ds) const;

protected:
  template <std::size_t... Is>
  auto _construct(const std::array<std::string, sizeof...(Vals)> &names,
                  std::index_sequence<Is...>) {
    return std::make_tuple(column<Vals>(names[Is])...);
  }

  template <typename DS, std::size_t... Is>
  auto _read(dataset::opened<DS> &ds, std::index_sequence<Is...>) const;

protected:
  std::tuple<column<Vals>...> m_columns;
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

template <typename... Vals>
template <typename... Names>
ana::dataset::columns<Vals...>::columns(Names const &...names)
    : m_columns(_construct(std::array<std::string, sizeof...(Vals)>{names...},
                           std::make_index_sequence<sizeof...(Vals)>{})) {}

template <typename... Vals>
ana::dataset::columns<Vals...>::columns(
    std::array<std::string, sizeof...(Vals)> const &names)
    : m_columns(
          _construct(names, std::make_index_sequence<sizeof...(Vals)>{})) {}

template <typename... Vals>
template <typename DS>
auto ana::dataset::columns<Vals...>::_read(ana::dataset::opened<DS> &ds) const {
  return this->_read(ds, std::make_index_sequence<sizeof...(Vals)>{});
}

template <typename... Vals>
template <typename DS, std::size_t... Is>
auto ana::dataset::columns<Vals...>::_read(ana::dataset::opened<DS> &ds,
                                           std::index_sequence<Is...>) const {
  return std::make_tuple(std::get<Is>(m_columns)._read(ds)...);
}