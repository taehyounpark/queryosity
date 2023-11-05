#pragma once

#include <map>

#include "dataflow.h"
#include "lazy.h"

namespace ana {

template <typename DS> class dataflow::reader {

public:
  reader(dataflow &df, DS &ds);
  ~reader() = default;

  template <typename Val> auto read_column(const std::string &name) {
    // Simulating reading of column
    // std::cout << "Reading " << name << " as type " << typeid(Val).name() <<
    // "\n"; return Val(); // return dummy value
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

  template <typename Val>
  auto read(const std::string &nomimal_column_name,
            std::map<std::string, std::string> const &varied_columns) {

    using varied_type = typename lazy<read_column_t<DS, Val>>::varied;

    auto syst = varied_type(this->read_column<Val>(nomimal_column_name));
    for (const auto &[variation_name, varied_column_name] : varied_columns) {
      syst.set_variation(variation_name,
                         this->read_column<Val>(varied_column_name));
    }
    return syst;
  }

protected:
  dataflow *m_df;
  dataset::input<DS> *m_ds;
};

} // namespace ana

template <typename DS>
ana::dataflow::reader<DS>::reader(ana::dataflow &df, DS &ds)
    : m_df(&df), m_ds(&ds) {}