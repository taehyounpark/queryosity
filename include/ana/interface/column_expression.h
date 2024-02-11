#pragma once

#include <string>
#include <tuple>

#include "column.h"

namespace ana {

class dataflow;

template <typename T> class lazy;

class selection;

template <typename Expr> class column::expression {

public:
  using function_type = decltype(std::function(std::declval<Expr>()));

public:
  expression(Expr expr);
  ~expression() = default;

  auto _equate(dataflow &df) const;

  template <typename Sel> auto _select(dataflow &df) const;

  template <typename Sel>
  auto _select(dataflow &df, lazy<selection> const &presel) const;

protected:
  function_type m_expression;
};

} // namespace ana

#include "dataflow.h"
#include "lazy.h"
#include "selection.h"

template <typename Expr>
ana::column::expression<Expr>::expression(Expr expr) : m_expression(expr) {}

template <typename Expr>
auto ana::column::expression<Expr>::_equate(ana::dataflow &df) const {
  return df._equate(this->m_expression);
}

template <typename Expr>
template <typename Sel>
auto ana::column::expression<Expr>::_select(ana::dataflow &df) const {
  return df._select<Sel>(this->m_expression);
}

template <typename Expr>
template <typename Sel>
auto ana::column::expression<Expr>::_select(
    ana::dataflow &df, lazy<selection> const &presel) const {
  return df._select<Sel>(presel, this->m_expression);
}