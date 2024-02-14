#pragma once

#include <set>
#include <type_traits>
#include <unordered_map>
#include <utility>

#include "lazy.h"
#include "systematic_resolver.h"

#define DECLARE_LAZY_VARIED_BINARY_OP(op_symbol)                               \
  template <typename Arg>                                                      \
  auto operator op_symbol(Arg &&b) const->typename lazy<                       \
      typename decltype(std::declval<lazy<Act>>().operator op_symbol(          \
          std::forward<Arg>(b).nominal()))::operation_type>::varied;
#define DEFINE_LAZY_VARIED_BINARY_OP(op_symbol)                                \
  template <typename Act>                                                      \
  template <typename Arg>                                                      \
  auto ana::lazy<Act>::varied::operator op_symbol(Arg &&b) const->             \
      typename lazy<                                                           \
          typename decltype(std::declval<lazy<Act>>().operator op_symbol(      \
              std::forward<Arg>(b).nominal()))::operation_type>::varied {      \
    auto syst = typename lazy<                                                 \
        typename decltype(std::declval<lazy<Act>>().operator op_symbol(        \
            std::forward<Arg>(b).nominal()))::operation_type>::                \
        varied(this->nominal().operator op_symbol(                             \
            std::forward<Arg>(b).nominal()));                                  \
    for (auto const &var_name :                                                \
         systematic::list_all_variation_names(*this, std::forward<Arg>(b))) {  \
      syst.set_variation(var_name,                                             \
                         variation(var_name).operator op_symbol(               \
                             std::forward<Arg>(b).variation(var_name)));       \
    }                                                                          \
    return syst;                                                               \
  }
#define DECLARE_LAZY_VARIED_UNARY_OP(op_symbol)                                \
  template <typename V = Act,                                                  \
            std::enable_if_t<ana::is_column_v<V>, bool> = false>               \
  auto operator op_symbol() const->typename lazy<                              \
      typename decltype(std::declval<lazy<V>>().                               \
                        operator op_symbol())::operation_type>::varied;
#define DEFINE_LAZY_VARIED_UNARY_OP(op_name, op_symbol)                        \
  template <typename Act>                                                      \
  template <typename V, std::enable_if_t<ana::is_column_v<V>, bool>>           \
  auto ana::lazy<Act>::varied::operator op_symbol() const->typename lazy<      \
      typename decltype(std::declval<lazy<V>>().                               \
                        operator op_symbol())::operation_type>::varied {       \
    auto syst = typename lazy<                                                 \
        typename decltype(std::declval<lazy<V>>().operator op_symbol())::      \
            operation_type>::varied(this->nominal().operator op_symbol());     \
    for (auto const &var_name : systematic::list_all_variation_names(*this)) { \
      syst.set_variation(var_name, variation(var_name).operator op_symbol());  \
    }                                                                          \
    return syst;                                                               \
  }

namespace ana {

/**
 * @brief Variations of a lazy operation to be performed in an dataflow.
 * @tparam T Input dataset type
 * @tparam U Actions to be performed lazily.
 * @details A `varied` node can be treated identical to a `lazy` one, except
 * that it contains multiple variations of the operation as dictated by the
 * analyzer that propagate through the rest of the analysis.
 */
template <typename Act>
class lazy<Act>::varied : public systematic::resolver<lazy<Act>> {

public:
  using operation_type = typename lazy<Act>::operation_type;

public:
  varied(lazy<Act> const &nom);
  ~varied() = default;

  virtual void set_variation(const std::string &var_name, lazy &&var) override;

  virtual lazy const &nominal() const override;
  virtual lazy const &variation(const std::string &var_name) const override;

  virtual bool has_variation(const std::string &var_name) const override;
  virtual std::set<std::string> list_variation_names() const override;

  template <typename Col, typename V = Act,
            std::enable_if_t<ana::is_selection_v<V>, bool> = false>
  auto filter(Col const &col) -> typename lazy<selection>::varied;

  template <typename Col, typename V = Act,
            std::enable_if_t<ana::is_selection_v<V>, bool> = false>
  auto weight(Col const &col) -> typename lazy<selection>::varied;

  template <typename Expr, typename... Args, typename V = Act,
            std::enable_if_t<ana::is_selection_v<V>, bool> = false>
  auto filter(column::expression<Expr> const &expr, Args &&...args) ->
      typename lazy<selection>::varied;

  template <typename Expr, typename... Args, typename V = Act,
            std::enable_if_t<ana::is_selection_v<V>, bool> = false>
  auto weight(column::expression<Expr> const &expr, Args &&...args) ->
      typename lazy<selection>::varied;

  template <typename Agg, typename V = Act,
            std::enable_if_t<ana::is_selection_v<V>, bool> = false>
  auto book(Agg &&agg);

  template <typename... Aggs, typename V = Act,
            std::enable_if_t<ana::is_selection_v<V>, bool> = false>
  auto book(Aggs &&...aggs);

  template <
      typename V = Act,
      std::enable_if_t<ana::counter::template has_output_v<V>, bool> = false>
  auto operator[](const std::string &var_name) const -> lazy<V>;

  DECLARE_LAZY_VARIED_UNARY_OP(-)
  DECLARE_LAZY_VARIED_UNARY_OP(!)

  DECLARE_LAZY_VARIED_BINARY_OP(+)
  DECLARE_LAZY_VARIED_BINARY_OP(-)
  DECLARE_LAZY_VARIED_BINARY_OP(*)
  DECLARE_LAZY_VARIED_BINARY_OP(/)
  DECLARE_LAZY_VARIED_BINARY_OP(<)
  DECLARE_LAZY_VARIED_BINARY_OP(>)
  DECLARE_LAZY_VARIED_BINARY_OP(<=)
  DECLARE_LAZY_VARIED_BINARY_OP(>=)
  DECLARE_LAZY_VARIED_BINARY_OP(==)
  DECLARE_LAZY_VARIED_BINARY_OP(&&)
  DECLARE_LAZY_VARIED_BINARY_OP(||)
  DECLARE_LAZY_VARIED_BINARY_OP([])

protected:
  lazy<Act> m_nom;
  std::unordered_map<std::string, lazy<Act>> m_var_map;
  std::set<std::string> m_var_names;
};

} // namespace ana

#include "selection.h"

template <typename Act>
ana::lazy<Act>::varied::varied(lazy<Act> const &nom)
    : systematic::resolver<lazy<Act>>::resolver(*nom.m_df), m_nom(nom) {}

template <typename Act>
void ana::lazy<Act>::varied::set_variation(const std::string &var_name,
                                           lazy &&var) {
  lockstep::call_slots(
      [var_name](operation *act) {
        act->systematic_mode().set_mode(false, var_name);
      },
      var);
  m_var_map.insert(std::make_pair(var_name, var));
  m_var_names.insert(var_name);
}

template <typename Act>
auto ana::lazy<Act>::varied::nominal() const -> lazy const & {
  return m_nom;
}

template <typename Act>
auto ana::lazy<Act>::varied::variation(const std::string &var_name) const
    -> lazy const & {
  return (this->has_variation(var_name) ? m_var_map.at(var_name) : m_nom);
}

template <typename Act>
bool ana::lazy<Act>::varied::has_variation(const std::string &var_name) const {
  return m_var_map.find(var_name) != m_var_map.end();
}

template <typename Act>
std::set<std::string> ana::lazy<Act>::varied::list_variation_names() const {
  return m_var_names;
}

template <typename Act>
template <typename Col, typename V,
          std::enable_if_t<ana::is_selection_v<V>, bool>>
auto ana::lazy<Act>::varied::filter(Col const &col) ->
    typename lazy<selection>::varied {

  using varied_type = typename lazy<selection>::varied;

  auto syst = varied_type(this->nominal().filter(col.nominal()));

  for (auto const &var_name :
       systematic::list_all_variation_names(*this, col)) {
    syst.set_variation(
        var_name, this->variation(var_name).filter(col.variation(var_name)));
  }
  return syst;
}

template <typename Act>
template <typename Col, typename V,
          std::enable_if_t<ana::is_selection_v<V>, bool>>
auto ana::lazy<Act>::varied::weight(Col const &col) ->
    typename lazy<selection>::varied {

  using varied_type = typename lazy<selection>::varied;

  auto syst = varied_type(this->nominal().weight(col.nominal()));

  for (auto const &var_name :
       systematic::list_all_variation_names(*this, col)) {
    syst.set_variation(
        var_name, this->variation(var_name).weight(col.variation(var_name)));
  }
  return syst;
}

template <typename Act>
template <typename Expr, typename... Args, typename V,
          std::enable_if_t<ana::is_selection_v<V>, bool>>
auto ana::lazy<Act>::varied::filter(ana::column::expression<Expr> const &expr,
                                    Args &&...args) ->
    typename lazy<selection>::varied {

  using varied_type = typename lazy<selection>::varied;

  auto syst = varied_type(
      this->nominal().filter(expr, std::forward<Args>(args).nominal()...));

  for (auto const &var_name : systematic::list_all_variation_names(
           *this, std::forward<Args>(args)...)) {
    syst.set_variation(
        var_name, this->variation(var_name).filter(
                      expr, std::forward<Args>(args).variation(var_name)...));
  }
  return syst;
}

template <typename Act>
template <typename Expr, typename... Args, typename V,
          std::enable_if_t<ana::is_selection_v<V>, bool>>
auto ana::lazy<Act>::varied::weight(ana::column::expression<Expr> const &expr,
                                    Args &&...args) ->
    typename lazy<selection>::varied {

  using varied_type = typename lazy<selection>::varied;

  auto syst = varied_type(
      this->nominal().weight(expr, std::forward<Args>(args).nominal()...));

  for (auto const &var_name : systematic::list_all_variation_names(
           *this, std::forward<Args>(args)...)) {
    syst.set_variation(
        var_name, this->variation(var_name).weight(
                      expr, std::forward<Args>(args).variation(var_name)...));
  }
  return syst;
}

template <typename Act>
template <typename Agg, typename V,
          std::enable_if_t<ana::is_selection_v<V>, bool>>
auto ana::lazy<Act>::varied::book(Agg &&agg) {
  return agg.book(*this);
}

template <typename Act>
template <typename... Aggs, typename V,
          std::enable_if_t<ana::is_selection_v<V>, bool>>
auto ana::lazy<Act>::varied::book(Aggs &&...aggs) {
  return std::make_tuple((aggs.book(*this), ...));
}

template <typename Act>
template <typename V,
          std::enable_if_t<ana::counter::template has_output_v<V>, bool>>
auto ana::lazy<Act>::varied::operator[](const std::string &var_name) const
    -> lazy<V> {
  if (!this->has_variation(var_name)) {
    throw std::out_of_range("variation does not exist");
  }
  return this->variation(var_name);
}

DEFINE_LAZY_VARIED_UNARY_OP(minus, -)
DEFINE_LAZY_VARIED_UNARY_OP(logical_not, !)

DEFINE_LAZY_VARIED_BINARY_OP(+)
DEFINE_LAZY_VARIED_BINARY_OP(-)
DEFINE_LAZY_VARIED_BINARY_OP(*)
DEFINE_LAZY_VARIED_BINARY_OP(/)
DEFINE_LAZY_VARIED_BINARY_OP(<)
DEFINE_LAZY_VARIED_BINARY_OP(>)
DEFINE_LAZY_VARIED_BINARY_OP(<=)
DEFINE_LAZY_VARIED_BINARY_OP(>=)
DEFINE_LAZY_VARIED_BINARY_OP(==)
DEFINE_LAZY_VARIED_BINARY_OP(&&)
DEFINE_LAZY_VARIED_BINARY_OP(||)
DEFINE_LAZY_VARIED_BINARY_OP([])