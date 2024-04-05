#pragma once

#include <string>
#include <tuple>

#include "column.h"
#include "column_equation.h"

namespace queryosity {

class dataflow;

template <typename T> class lazy;

namespace selection {

class node;

}

namespace column {

/**
 * @brief Define a column evaluated out of an expression.
 */
template <typename Expr> struct expression {

public:
  using function_type = decltype(std::function(std::declval<Expr>()));
  using equation_type = equation_t<Expr>;

public:
  expression(Expr expr);
  ~expression() = default;

  auto _equate(dataflow &df) const;

  template <typename Sel> auto _select(dataflow &df) const;

  template <typename Sel>
  auto _select(dataflow &df, lazy<selection::node> const &presel) const;

protected:
  function_type m_expression;
};

} // namespace column

} // namespace queryosity

#include "dataflow.h"
#include "lazy.h"
#include "selection.h"

template <typename Expr>
queryosity::column::expression<Expr>::expression(Expr expr)
    : m_expression(expr) {}

template <typename Expr>
auto queryosity::column::expression<Expr>::_equate(
    queryosity::dataflow &df) const {
  return df._equate(this->m_expression);
}

template <typename Expr>
template <typename Sel>
auto queryosity::column::expression<Expr>::_select(
    queryosity::dataflow &df) const {
  return df._select<Sel>(this->m_expression);
}

template <typename Expr>
template <typename Sel>
auto queryosity::column::expression<Expr>::_select(
    queryosity::dataflow &df, lazy<selection::node> const &presel) const {
  return df._select<Sel>(presel, this->m_expression);
}