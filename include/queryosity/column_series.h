#pragma once

#include "column.h"
#include "dataflow.h"
#include "lazy.h"
#include "lazy_varied.h"
#include "query_series.h"
#include "selection.h"

namespace queryosity {

namespace column {

/**
 * @brief Argumnet for column series.
 * @tparam Col (Varied) lazy column node.
 * @todo C++20: Use concept to require lazy<column<Val>(::varied)>.
 */
template <typename Col> class series {

public:
  using value_type = column::value_t<typename Col::action_type>;

public:
  series(Col const &col);
  ~series() = default;

  auto _get(lazy<selection::node> &sel) const;

  auto _get(lazy<selection::node>::varied &sel) const ->
      typename lazy<query::series<value_type>>::varied;

protected:
  Col m_column;
};

} // namespace column

} // namespace queryosity

template <typename Col>
queryosity::column::series<Col>::series(Col const &col) : m_column(col){};

template <typename Col>
auto queryosity::column::series<Col>::_get(lazy<selection::node> &sel) const {
  auto df = sel.m_df;
  return df->make(query::plan<query::series<value_type>>())
      .fill(m_column)
      .book(sel);
}

template <typename Col>
auto queryosity::column::series<Col>::_get(lazy<selection::node>::varied &sel)
    const -> typename lazy<query::series<value_type>>::varied {
  auto df = sel.nominal().m_df;
  return df->make(query::plan<query::series<value_type>>())
      .fill(m_column)
      .book(sel);
}