#pragma once

#include <set>
#include <type_traits>
#include <unordered_map>
#include <utility>

#include "detail.h"
#include "lazy.h"
#include "systematic_resolver.h"

namespace queryosity {

template <typename T> class varied;

/**
 * @ingroup api
 * @brief Variations of a lazy action.
 * @tparam Action Action to be performed.
 * @details A varied lazy action encapsulates independent nominal and variation
 * instances of of a lazy action, which are implicitly propagated through all
 * downstream actions that it participates in. In other words, a varied node
 * behaves functionally identical to a nominal-only one.
 */
template <typename Act>
class varied<lazy<Act>> : public dataflow::node,
                          public systematic::resolver<lazy<Act>> {

public:
  using action_type = typename lazy<Act>::action_type;

  template <typename> friend class lazy;
  template <typename> friend class varied;

public:
  varied(lazy<Act> nom);
  virtual ~varied() = default;

  varied(varied const &) = default;
  varied &operator=(varied const &) = default;

  varied(varied &&) = default;
  varied &operator=(varied &&) = default;

  template <typename Derived> varied(varied<lazy<Derived>> const &);
  template <typename Derived> varied &operator=(varied<lazy<Derived>> const &);

  virtual void set_variation(const std::string &var_name,
                             lazy<Act> var) final override;

  virtual lazy<Act> &nominal() final override;
  virtual lazy<Act> &variation(const std::string &var_name) final override;
  virtual lazy<Act> const &nominal() const final override;
  virtual lazy<Act> const &
  variation(const std::string &var_name) const final override;

  virtual bool has_variation(const std::string &var_name) const final override;
  virtual std::set<std::string> get_variation_names() const final override;

  /**
   * @brief Compound a cut to this selection.
   * @Col (Varied) lazy input column type.
   * @parma[in] col (Varied) lazy input column used as cut decision.
   * @return Varied lazy selection.
   */
  template <typename Col, typename V = Act,
            std::enable_if_t<queryosity::is_selection_v<V>, bool> = false>
  auto filter(Col const &col) -> varied<lazy<selection::node>>;

  /**
   * @brief Compound a weight to this selection.
   * @Col (Varied) lazy input column type.
   * @parma[in] col (Varied) lazy input column used as cut decision.
   * @return Varied lazy selection.
   */
  template <typename Col, typename V = Act,
            std::enable_if_t<queryosity::is_selection_v<V>, bool> = false>
  auto weight(Col const &col) -> varied<lazy<selection::node>>;

  template <typename Expr, typename V = Act,
            std::enable_if_t<queryosity::is_selection_v<V>, bool> = false>
  auto filter(column::expression<Expr> const &expr) -> varied<
      todo<selection::applicator<selection::cut, column::equation_t<Expr>>>>;

  template <typename Expr, typename V = Act,
            std::enable_if_t<queryosity::is_selection_v<V>, bool> = false>
  auto weight(column::expression<Expr> const &expr) -> varied<
      todo<selection::applicator<selection::weight, column::equation_t<Expr>>>>;

  template <typename Agg, typename V = Act,
            std::enable_if_t<queryosity::is_selection_v<V>, bool> = false>
  auto book(Agg &&agg);

  template <typename... Aggs, typename V = Act,
            std::enable_if_t<queryosity::is_selection_v<V>, bool> = false>
  auto book(Aggs &&...aggs);

  template <
      typename V = Act,
      std::enable_if_t<queryosity::query::is_aggregation_v<V>, bool> = false>
  auto operator[](const std::string &var_name) -> lazy<V> &;
  template <
      typename V = Act,
      std::enable_if_t<queryosity::query::is_aggregation_v<V>, bool> = false>
  auto operator[](const std::string &var_name) const -> lazy<V> const &;

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

} // namespace queryosity

#include "selection.h"

template <typename Act>
queryosity::varied<queryosity::lazy<Act>>::varied(queryosity::lazy<Act> nom)
    : dataflow::node(*nom.m_df), m_nom(std::move(nom)) {}

template <typename Act>
template <typename Derived>
queryosity::varied<queryosity::lazy<Act>>::varied(
    varied<queryosity::lazy<Derived>> const &other) {
  this->m_df = other.m_df;
  this->m_var_names = other.m_var_names;
  for (auto const &var : other.m_var_map) {
    m_var_map.insert(var);
  }
}

template <typename Act>
template <typename Derived>
queryosity::varied<queryosity::lazy<Act>> &
queryosity::varied<queryosity::lazy<Act>>::operator=(
    varied<queryosity::lazy<Derived>> const &other) {
  this->m_df = other.m_df;
  this->m_var_names = other.m_var_names;
  for (auto const &var : other.m_var_map) {
    m_var_map.insert(var);
  }
  return *this;
}

template <typename Act>
void queryosity::varied<queryosity::lazy<Act>>::set_variation(
    const std::string &var_name, queryosity::lazy<Act> var) {
  ensemble::invoke(
      [var_name](action *act) { act->set_variation_name(var_name); },
      var.get_slots());
  m_var_map.insert(std::make_pair(var_name, std::move(var)));
  m_var_names.insert(var_name);
}

template <typename Act>
auto queryosity::varied<queryosity::lazy<Act>>::nominal()
    -> queryosity::lazy<Act> & {
  return this->m_nom;
}

template <typename Act>
auto queryosity::varied<queryosity::lazy<Act>>::variation(
    const std::string &var_name) -> queryosity::lazy<Act> & {
  return (this->has_variation(var_name) ? m_var_map.at(var_name) : m_nom);
}

template <typename Act>
auto queryosity::varied<queryosity::lazy<Act>>::nominal() const
    -> queryosity::lazy<Act> const & {
  return this->m_nom;
}

template <typename Act>
auto queryosity::varied<queryosity::lazy<Act>>::variation(
    const std::string &var_name) const -> queryosity::lazy<Act> const & {
  return (this->has_variation(var_name) ? m_var_map.at(var_name) : m_nom);
}

template <typename Act>
bool queryosity::varied<queryosity::lazy<Act>>::has_variation(
    const std::string &var_name) const {
  return m_var_map.find(var_name) != m_var_map.end();
}

template <typename Act>
std::set<std::string>
queryosity::varied<queryosity::lazy<Act>>::get_variation_names() const {
  return m_var_names;
}

template <typename Act>
template <typename Col, typename V,
          std::enable_if_t<queryosity::is_selection_v<V>, bool>>
auto queryosity::varied<queryosity::lazy<Act>>::filter(Col const &col)
    -> varied<lazy<selection::node>> {

  using varied_type = varied<lazy<selection::node>>;

  auto syst = varied_type(this->nominal().filter(col.nominal()));

  for (auto const &var_name : systematic::get_variation_names(*this, col)) {
    syst.set_variation(
        var_name, this->variation(var_name).filter(col.variation(var_name)));
  }
  return syst;
}

template <typename Act>
template <typename Col, typename V,
          std::enable_if_t<queryosity::is_selection_v<V>, bool>>
auto queryosity::varied<queryosity::lazy<Act>>::weight(Col const &col)
    -> varied<lazy<selection::node>> {

  using varied_type = varied<lazy<selection::node>>;

  auto syst = varied_type(this->nominal().weight(col.nominal()));

  for (auto const &var_name : systematic::get_variation_names(*this, col)) {
    syst.set_variation(
        var_name, this->variation(var_name).weight(col.variation(var_name)));
  }
  return syst;
}

template <typename Act>
template <typename Expr, typename V,
          std::enable_if_t<queryosity::is_selection_v<V>, bool>>
auto queryosity::varied<queryosity::lazy<Act>>::filter(
    queryosity::column::expression<Expr> const &expr)
    -> varied<
        todo<selection::applicator<selection::cut, column::equation_t<Expr>>>> {

  using varied_type = varied<
      todo<selection::applicator<selection::cut, column::equation_t<Expr>>>>;

  auto syst = varied_type(this->nominal().filter(expr));

  for (auto const &var_name : systematic::get_variation_names(*this)) {
    syst.set_variation(var_name, this->variation(var_name).filter(expr));
  }
  return syst;
}

template <typename Act>
template <typename Expr, typename V,
          std::enable_if_t<queryosity::is_selection_v<V>, bool>>
auto queryosity::varied<queryosity::lazy<Act>>::weight(
    queryosity::column::expression<Expr> const &expr)
    -> varied<todo<
        selection::applicator<selection::weight, column::equation_t<Expr>>>> {

  using varied_type = varied<
      todo<selection::applicator<selection::weight, column::equation_t<Expr>>>>;

  auto syst = varied_type(this->nominal().weight(expr));

  for (auto const &var_name : systematic::get_variation_names(*this)) {
    syst.set_variation(var_name, this->variation(var_name).weight(expr));
  }
  return syst;
}

template <typename Act>
template <typename Agg, typename V,
          std::enable_if_t<queryosity::is_selection_v<V>, bool>>
auto queryosity::varied<queryosity::lazy<Act>>::book(Agg &&agg) {
  return agg.at(*this);
}

template <typename Act>
template <typename... Aggs, typename V,
          std::enable_if_t<queryosity::is_selection_v<V>, bool>>
auto queryosity::varied<queryosity::lazy<Act>>::book(Aggs &&...aggs) {
  return std::make_tuple((aggs.at(*this), ...));
}

template <typename Act>
template <typename V,
          std::enable_if_t<queryosity::query::is_aggregation_v<V>, bool>>
auto queryosity::varied<queryosity::lazy<Act>>::operator[](
    const std::string &var_name) -> queryosity::lazy<V> & {
  if (!this->has_variation(var_name)) {
    throw std::out_of_range("variation does not exist");
  }
  return this->variation(var_name);
}

template <typename Act>
template <typename V,
          std::enable_if_t<queryosity::query::is_aggregation_v<V>, bool>>
auto queryosity::varied<queryosity::lazy<Act>>::operator[](
    const std::string &var_name) const -> queryosity::lazy<V> const & {
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