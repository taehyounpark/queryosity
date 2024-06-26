#pragma once

#include <string>
#include <tuple>

#include "column.hpp"
#include "column_equation.hpp"

namespace queryosity {

class dataflow;

template <typename T> class lazy;

namespace selection {

class node;

}

namespace column {

/**
 * @brief Argument to define a column evaluated out of an expression in the
 * dataflow.
 * @tparam Expr Concrete type of C++ function, functor, or lambda.
 */
template <typename Expr> struct expression {

public:
  using function_type = decltype(std::function(std::declval<Expr>()));
  using equation_type = equation_t<Expr>;

public:
  /**
   * @brief Argument constructor.
   * @param[in] expr The callable expression.
   */
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

#include "dataflow.hpp"
#include "lazy.hpp"
#include "selection.hpp"

template <typename Expr>
queryosity::column::expression<Expr>::expression(Expr expr)
    : m_expression(std::move(expr)) {}

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