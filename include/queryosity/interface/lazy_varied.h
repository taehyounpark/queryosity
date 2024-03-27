#pragma once

#include <set>
#include <type_traits>
#include <unordered_map>
#include <utility>

#include "detail.h"
#include "lazy.h"
#include "systematic_resolver.h"

namespace queryosity {

/**
 * @ingroup api
 * @brief Variations of a lazy action.
 * @details A varied lazy node encapsulates independent nominal and variations
 * of a lazy action.
 * @tparam Action Action to be performed.
 */
template <typename Act>
class lazy<Act>::varied : public dataflow::node,
                          public systematic::resolver<lazy<Act>> {

public:
  using action_type = typename lazy<Act>::action_type;

public:
  varied(lazy<Act> nom);
  ~varied() = default;

  varied(varied &&) = default;
  varied &operator=(varied &&) = default;

  virtual void set_variation(const std::string &var_name, lazy var) override;

  virtual lazy &nominal() override;
  virtual lazy &variation(const std::string &var_name) override;
  virtual lazy const &nominal() const override;
  virtual lazy const &variation(const std::string &var_name) const override;

  virtual bool has_variation(const std::string &var_name) const override;
  virtual std::set<std::string> get_variation_names() const override;

  /**
   * Apply a filter.
   */
  template <typename Col, typename V = Act,
            std::enable_if_t<queryosity::is_selection_v<V>, bool> = false>
  auto filter(Col const &col) -> typename lazy<selection::node>::varied;

  template <typename Col, typename V = Act,
            std::enable_if_t<queryosity::is_selection_v<V>, bool> = false>
  auto weight(Col const &col) -> typename lazy<selection::node>::varied;

  template <typename Expr, typename... Args, typename V = Act,
            std::enable_if_t<queryosity::is_selection_v<V>, bool> = false>
  auto filter(column::expression<Expr> const &expr, Args &&...args) ->
      typename lazy<selection::node>::varied;

  template <typename Expr, typename... Args, typename V = Act,
            std::enable_if_t<queryosity::is_selection_v<V>, bool> = false>
  auto weight(column::expression<Expr> const &expr, Args &&...args) ->
      typename lazy<selection::node>::varied;

  template <typename Agg, typename V = Act,
            std::enable_if_t<queryosity::is_selection_v<V>, bool> = false>
  auto book(Agg &&agg);

  template <typename... Aggs, typename V = Act,
            std::enable_if_t<queryosity::is_selection_v<V>, bool> = false>
  auto book(Aggs &&...aggs);

  template <typename V = Act,
            std::enable_if_t<queryosity::query::is_aggregation_v<V>,
                             bool> = false>
  auto operator[](const std::string &var_name) -> lazy<V> &;
  template <typename V = Act,
            std::enable_if_t<queryosity::query::is_aggregation_v<V>,
                             bool> = false>
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
queryosity::lazy<Act>::varied::varied(lazy<Act> nom)
    : dataflow::node(*nom.m_df), m_nom(std::move(nom)) {}

template <typename Act>
void queryosity::lazy<Act>::varied::set_variation(const std::string &var_name,
                                                  lazy var) {
  ensemble::call([var_name](action *act) { act->set_variation_name(var_name); },
                 var.get_slots());
  m_var_map.insert(std::make_pair(var_name, std::move(var)));
  m_var_names.insert(var_name);
}

template <typename Act>
auto queryosity::lazy<Act>::varied::nominal() -> lazy & {
  return this->m_nom;
}

template <typename Act>
auto queryosity::lazy<Act>::varied::variation(const std::string &var_name)
    -> lazy & {
  return (this->has_variation(var_name) ? m_var_map.at(var_name) : m_nom);
}

template <typename Act>
auto queryosity::lazy<Act>::varied::nominal() const -> lazy const & {
  return this->m_nom;
}

template <typename Act>
auto queryosity::lazy<Act>::varied::variation(const std::string &var_name) const
    -> lazy const & {
  return (this->has_variation(var_name) ? m_var_map.at(var_name) : m_nom);
}

template <typename Act>
bool queryosity::lazy<Act>::varied::has_variation(
    const std::string &var_name) const {
  return m_var_map.find(var_name) != m_var_map.end();
}

template <typename Act>
std::set<std::string>
queryosity::lazy<Act>::varied::get_variation_names() const {
  return m_var_names;
}

template <typename Act>
template <typename Col, typename V,
          std::enable_if_t<queryosity::is_selection_v<V>, bool>>
auto queryosity::lazy<Act>::varied::filter(Col const &col) ->
    typename lazy<selection::node>::varied {

  using varied_type = typename lazy<selection::node>::varied;

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
auto queryosity::lazy<Act>::varied::weight(Col const &col) ->
    typename lazy<selection::node>::varied {

  using varied_type = typename lazy<selection::node>::varied;

  auto syst = varied_type(this->nominal().weight(col.nominal()));

  for (auto const &var_name : systematic::get_variation_names(*this, col)) {
    syst.set_variation(
        var_name, this->variation(var_name).weight(col.variation(var_name)));
  }
  return syst;
}

template <typename Act>
template <typename Expr, typename... Args, typename V,
          std::enable_if_t<queryosity::is_selection_v<V>, bool>>
auto queryosity::lazy<Act>::varied::filter(
    queryosity::column::expression<Expr> const &expr, Args &&...args) ->
    typename lazy<selection::node>::varied {

  using varied_type = typename lazy<selection::node>::varied;

  auto syst = varied_type(
      this->nominal().filter(expr, std::forward<Args>(args).nominal()...));

  for (auto const &var_name :
       systematic::get_variation_names(*this, std::forward<Args>(args)...)) {
    syst.set_variation(
        var_name, this->variation(var_name).filter(
                      expr, std::forward<Args>(args).variation(var_name)...));
  }
  return syst;
}

template <typename Act>
template <typename Expr, typename... Args, typename V,
          std::enable_if_t<queryosity::is_selection_v<V>, bool>>
auto queryosity::lazy<Act>::varied::weight(
    queryosity::column::expression<Expr> const &expr, Args &&...args) ->
    typename lazy<selection::node>::varied {

  using varied_type = typename lazy<selection::node>::varied;

  auto syst = varied_type(
      this->nominal().weight(expr, std::forward<Args>(args).nominal()...));

  for (auto const &var_name :
       systematic::get_variation_names(*this, std::forward<Args>(args)...)) {
    syst.set_variation(
        var_name, this->variation(var_name).weight(
                      expr, std::forward<Args>(args).variation(var_name)...));
  }
  return syst;
}

template <typename Act>
template <typename Agg, typename V,
          std::enable_if_t<queryosity::is_selection_v<V>, bool>>
auto queryosity::lazy<Act>::varied::book(Agg &&agg) {
  return agg.book(*this);
}

template <typename Act>
template <typename... Aggs, typename V,
          std::enable_if_t<queryosity::is_selection_v<V>, bool>>
auto queryosity::lazy<Act>::varied::book(Aggs &&...aggs) {
  return std::make_tuple((aggs.book(*this), ...));
}

template <typename Act>
template <
    typename V,
    std::enable_if_t<queryosity::query::is_aggregation_v<V>, bool>>
auto queryosity::lazy<Act>::varied::operator[](const std::string &var_name)
    -> lazy<V> & {
  if (!this->has_variation(var_name)) {
    throw std::out_of_range("variation does not exist");
  }
  return this->variation(var_name);
}

template <typename Act>
template <
    typename V,
    std::enable_if_t<queryosity::query::is_aggregation_v<V>, bool>>
auto queryosity::lazy<Act>::varied::operator[](
    const std::string &var_name) const -> lazy<V> const & {
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