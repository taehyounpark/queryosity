#pragma once

#include <map>

#include "dataflow.h"
#include "systematic.h"

namespace ana {

template <typename DS> class dataset::reader {

public:
  reader(dataflow &df, DS &ds);
  ~reader() = default;

  template <typename Val> auto read_column(const std::string &name) {
    return m_df->read<DS, Val>(*m_ds, name);
  }

  template <typename... Vals, size_t... Is>
  auto read_columns(const std::array<std::string, sizeof...(Vals)> &names,
                    std::index_sequence<Is...>) {
    return std::make_tuple(
        read_column<typename std::tuple_element_t<Is, std::tuple<Vals...>>>(
            names[Is])...);
  }

  template <typename... Vals>
  auto read(const std::array<std::string, sizeof...(Vals)> &names) {
    return read_columns<Vals...>(names, std::index_sequence_for<Vals...>{});
  }

  template <typename Val> auto read(const std::string &name) {
    return this->read_column<Val>(name);
  }

  template <typename Col>
  auto vary(lazy<Col> const &nom,
            systematic::variation<std::string> const &var);

  template <typename Col, typename... Vars>
  auto vary(lazy<Col> const &nom, systematic::variation<Vars> const &...vars);

protected:
  dataflow *m_df;
  dataset::input<DS> *m_ds;
};

} // namespace ana

#include "lazy.h"
#include "lazy_varied.h"
#include "systematic_variation.h"

template <typename DS>
ana::dataset::reader<DS>::reader(ana::dataflow &df, DS &ds)
    : m_df(&df), m_ds(&ds) {}

template <typename DS>
template <typename Col>
auto ana::dataset::reader<DS>::vary(
    lazy<Col> const &nom, systematic::variation<std::string> const &var) {
  using nom_val_t = cell_value_t<Col>;
  typename lazy<Col>::varied varied_column(nom);
  varied_column.set_variation(
      var.name(), this->read_column<nom_val_t>(std::get<0>(var.args())));
  return varied_column;
}

template <typename DS>
template <typename Col, typename... Vars>
auto ana::dataset::reader<DS>::vary(
    lazy<Col> const &nom, systematic::variation<Vars> const &...vars) {
  using nom_val_t = cell_value_t<Col>;
  typename lazy<Col>::varied varied_column(nom);
  (varied_column.set_variation(
       vars.name(), this->read_column<nom_val_t>(std::get<0>(vars.args()))),
   ...);
  return varied_column;
}