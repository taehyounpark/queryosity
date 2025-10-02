#pragma once

#include <map>

#include "dataflow.hpp"
#include "systematic.hpp"

namespace queryosity {

template <typename DS> class dataflow::input {

public:
  input(dataflow &df, DS &ds);
  ~input() = default;

  template <typename Val> auto _read(const std::string &name) {
    return m_df->_read<DS, Val>(*m_ds, name);
  }

  template <typename Val>
  auto read(dataset::column<Val> const &col)
      -> lazy<queryosity::column::valued<Val>>;

template <typename... Vals>
auto read(dataset::column<Vals> const &...cols)
    -> std::tuple<lazy<queryosity::column::valued<Vals>>...>;

  template <typename Val>
  auto vary(dataset::column<Val> const &col,
            std::map<std::string, std::string> const &vars);

protected:
  dataflow *m_df;
  dataset::reader<DS> *m_ds;
};

} // namespace queryosity

#include "dataset_column.hpp"
#include "lazy.hpp"
#include "lazy_varied.hpp"

template <typename DS>
queryosity::dataflow::input<DS>::input(queryosity::dataflow &df, DS &ds)
    : m_df(&df), m_ds(&ds) {}

template <typename DS>
template <typename Val>
auto queryosity::dataflow::input<DS>::read(dataset::column<Val> const &col)
    -> lazy<queryosity::column::valued<Val>> {
  return col.template _read(*this);
}

template <typename DS>
template <typename... Vals>
auto queryosity::dataflow::input<DS>::read(
    dataset::column<Vals> const &...cols) -> std::tuple<lazy<queryosity::column::valued<Vals>>...> {
  return std::make_tuple(cols.template _read(*this)...);
}

template <typename DS>
template <typename Val>
auto queryosity::dataflow::input<DS>::vary(
    dataset::column<Val> const &col,
    std::map<std::string, std::string> const &vars) {
  auto nom = this->read(col);
  varied<decltype(nom)> varied_column(std::move(nom));
  for (auto const &var : vars) {
    varied_column.set_variation(var.first,
                                this->read(dataset::column<Val>(var.second)));
  }
  return varied_column;
}