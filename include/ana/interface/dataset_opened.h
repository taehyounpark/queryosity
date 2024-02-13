#pragma once

#include <map>

#include "dataflow.h"
#include "systematic.h"

namespace ana {

template <typename DS> class dataset::opened {

public:
  opened(dataflow &df, DS &ds);
  ~opened() = default;

  template <typename Val> auto _read(const std::string &name) {
    return m_df->_read<DS, Val>(*m_ds, name);
  }

  template <typename Val> auto read(dataset::column<Val> const &col);

  template <typename... Vals> auto read(dataset::columns<Vals...> const &cols);

  template <typename Val>
  auto vary(dataset::column<Val> const &nom,
            systematic::variation<std::string> const &var);

  template <typename Val, typename... Vars>
  auto vary(dataset::column<Val> const &nom,
            systematic::variation<Vars> const &...vars);

protected:
  dataflow *m_df;
  dataset::source<DS> *m_ds;
};

} // namespace ana

#include "dataset_column.h"
#include "lazy.h"
#include "lazy_varied.h"
#include "systematic_variation.h"

template <typename DS>
ana::dataset::opened<DS>::opened(ana::dataflow &df, DS &ds)
    : m_df(&df), m_ds(&ds) {}

template <typename DS>
template <typename Val>
auto ana::dataset::opened<DS>::read(dataset::column<Val> const &col) {
  return col.template _read(*this);
}

template <typename DS>
template <typename... Vals>
auto ana::dataset::opened<DS>::read(dataset::columns<Vals...> const &cols) {
  return cols.template _read(*this);
}

template <typename DS>
template <typename Val>
auto ana::dataset::opened<DS>::vary(
    dataset::column<Val> const &col,
    systematic::variation<std::string> const &var) {
  auto nom = this->read(col);
  typename decltype(nom)::varied varied_column(nom);
  varied_column.set_variation(
      var.name(), this->read(dataset::column<Val>(std::get<0>(var.args()))));
  return varied_column;
}

template <typename DS>
template <typename Val, typename... Vars>
auto ana::dataset::opened<DS>::vary(
    dataset::column<Val> const &col,
    systematic::variation<Vars> const &...vars) {
  auto nom = this->read(col);
  typename decltype(nom)::varied varied_column(nom);
  (varied_column.set_variation(
       vars.name(), this->read(dataset::column<Val>(std::get<0>(vars.args())))),
   ...);
  return varied_column;
}