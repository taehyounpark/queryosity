#pragma once

#include "column.hpp"
#include "dataflow.hpp"
#include "lazy.hpp"
#include "lazy_varied.hpp"
#include "query_series.hpp"
#include "selection.hpp"

namespace queryosity {

namespace column {

/**
 * @brief Argumnet for column series.
 * @tparam Col (Varied) lazy column node.
 * @todo C++20: Use concept to require lazy<column<Val>(::varied)>.
 */
template <typename Col> struct series {

public:
  using value_type = column::value_t<typename Col::action_type>;

public:
  series(Col const &col);
  ~series() = default;

  auto make(dataflow &df) const;

  auto make(lazy<selection::node> &sel) const;

  auto make(varied<lazy<selection::node>> &sel) const ->
      varied<lazy<query::series<value_type>>>;

protected:
  Col m_column;
};

} // namespace column

} // namespace queryosity

template <typename Col>
queryosity::column::series<Col>::series(Col const &col) : m_column(col){};

template <typename Col>
auto queryosity::column::series<Col>::make(dataflow &df) const {
  return df.get(query::result<query::series<value_type>>()).fill(m_column);
}

template <typename Col>
auto queryosity::column::series<Col>::make(lazy<selection::node> &sel) const {
  auto df = sel.m_df;
  return df->get(query::result<query::series<value_type>>())
      .fill(m_column)
      .at(sel);
}

template <typename Col>
auto queryosity::column::series<Col>::make(varied<lazy<selection::node>> &sel)
    const -> varied<lazy<query::series<value_type>>> {
  auto df = sel.nominal().m_df;
  return df->get(query::result<query::series<value_type>>())
      .fill(m_column)
      .at(sel);
}