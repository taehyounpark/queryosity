#pragma once

#include "dataflow.h"

namespace ana {

template <typename DS> class dataflow::reader {

public:
  reader(dataflow &df, std::unique_ptr<DS> ds);

  template <typename... Args> void open(Args &&...args);

  template <typename Val> auto read_column(const std::string &name) {
    // Simulating reading of column
    // std::cout << "Reading " << name << " as type " << typeid(Val).name() <<
    // "\n"; return Val(); // return dummy value
    return m_df->read<DS, Val>(*m_ds, name);
  }

  template <typename... Vals, size_t... Is>
  auto read_impl(const std::array<std::string, sizeof...(Vals)> &names,
                 std::index_sequence<Is...>) {
    return std::make_tuple(
        read_column<typename std::tuple_element_t<Is, std::tuple<Vals...>>>(
            names[Is])...);
  }

  template <typename... Vals>
  auto read(const std::array<std::string, sizeof...(Vals)> &names) {
    return read_impl<Vals...>(names, std::index_sequence_for<Vals...>{});
  }

protected:
  dataflow *m_df;
  std::unique_ptr<dataset::input<DS>> m_ds;
};

} // namespace ana

template <typename DS>
ana::dataflow::reader<DS>::reader(ana::dataflow &df, std::unique_ptr<DS> ds)
    : m_df(&df), m_ds(std::move(ds)) {}

template <typename DS>
template <typename... Args>
void ana::dataflow::reader<DS>::open(Args &&...args) {
  m_ds = std::make_unique<DS>(std::forward<Args>(args)...);
}