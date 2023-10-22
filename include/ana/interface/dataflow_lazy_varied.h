#pragma once

#include <set>
#include <type_traits>
#include <unordered_map>
#include <utility>

#include "dataflow_lazy.h"
#include "dataflow_systematic.h"

#define DECLARE_LAZY_VARIED_BINARY_OP(op_symbol)                               \
  template <typename Arg>                                                      \
  auto operator op_symbol(Arg &&b) const->typename lazy<                       \
      typename decltype(std::declval<lazy<Act>>().operator op_symbol(          \
          std::forward<Arg>(b).nominal()))::operation_type>::varied;
#define DEFINE_LAZY_VARIED_BINARY_OP(op_symbol)                                \
  template <typename Act>                                                      \
  template <typename Arg>                                                      \
  auto ana::dataflow::lazy<Act>::varied::operator op_symbol(Arg &&b) const->   \
      typename lazy<                                                           \
          typename decltype(std::declval<lazy<Act>>().operator op_symbol(      \
              std::forward<Arg>(b).nominal()))::operation_type>::varied {      \
    auto syst = typename lazy<                                                 \
        typename decltype(std::declval<lazy<Act>>().operator op_symbol(        \
            std::forward<Arg>(b).nominal()))::operation_type>::                \
        varied(this->nominal().operator op_symbol(                             \
            std::forward<Arg>(b).nominal()));                                  \
    for (auto const &var_name :                                                \
         list_all_variation_names(*this, std::forward<Arg>(b))) {              \
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
  auto ana::dataflow::lazy<Act>::varied::operator op_symbol() const->          \
      typename lazy<                                                           \
          typename decltype(std::declval<lazy<V>>().                           \
                            operator op_symbol())::operation_type>::varied {   \
    auto syst = typename lazy<                                                 \
        typename decltype(std::declval<lazy<V>>().operator op_symbol())::      \
            operation_type>::varied(this->nominal().operator op_symbol());     \
    for (auto const &var_name : list_all_variation_names(*this)) {             \
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
class dataflow::lazy<Act>::varied : public systematic<lazy<Act>> {

public:
  using operation_type = typename lazy<Act>::operation_type;

  template <typename... Args>
  using delayed_varied_selection_applicator_t =
      typename decltype(std::declval<dataflow>().filter(
          std::declval<std::string>(), std::declval<Args>()...))::varied;

public:
  varied(lazy<Act> const &nom);
  ~varied() = default;

  virtual void set_variation(const std::string &var_name, lazy &&var) override;

  virtual lazy const &nominal() const override;
  virtual lazy const &variation(const std::string &var_name) const override;

  virtual bool has_variation(const std::string &var_name) const override;
  virtual std::set<std::string> list_variation_names() const override;

  template <typename... Args, typename V = Act,
            std::enable_if_t<ana::is_selection_v<V>, bool> = false>
  auto filter(const std::string &name, Args &&...arguments)
      -> delayed_varied_selection_applicator_t<Args...>;

  template <typename... Args, typename V = Act,
            std::enable_if_t<ana::is_selection_v<V>, bool> = false>
  auto weight(const std::string &name, Args &&...arguments)
      -> delayed_varied_selection_applicator_t<Args...>;

  template <typename... Args, typename V = Act,
            std::enable_if_t<ana::is_selection_v<V>, bool> = false>
  auto channel(const std::string &name, Args &&...arguments)
      -> delayed_varied_selection_applicator_t<Args...>;

  template <typename Agg, typename V = Act,
            std::enable_if_t<ana::is_selection_v<V>, bool> = false>
  auto book(Agg &&agg);

  template <typename... Aggs, typename V = Act,
            std::enable_if_t<ana::is_selection_v<V>, bool> = false>
  auto book(Aggs &&...aggs);

  template <typename V = Act,
            std::enable_if_t<ana::is_column_v<V> ||
                                 ana::aggregation::template has_output_v<V>,
                             bool> = false>
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
ana::dataflow::lazy<Act>::varied::varied(lazy<Act> const &nom)
    : systematic<lazy<Act>>::systematic(*nom.m_df), m_nom(nom) {}

template <typename Act>
void ana::dataflow::lazy<Act>::varied::set_variation(
    const std::string &var_name, lazy &&var) {
  m_var_map.insert(std::make_pair(var_name, std::move(var)));
  m_var_names.insert(var_name);
}

template <typename Act>
auto ana::dataflow::lazy<Act>::varied::nominal() const -> lazy const & {
  return m_nom;
}

template <typename Act>
auto ana::dataflow::lazy<Act>::varied::variation(
    const std::string &var_name) const -> lazy const & {
  return (this->has_variation(var_name) ? m_var_map.at(var_name) : m_nom);
}

template <typename Act>
bool ana::dataflow::lazy<Act>::varied::has_variation(
    const std::string &var_name) const {
  return m_var_map.find(var_name) != m_var_map.end();
}

template <typename Act>
std::set<std::string>
ana::dataflow::lazy<Act>::varied::list_variation_names() const {
  return m_var_names;
}

template <typename Act>
template <typename... Args, typename V,
          std::enable_if_t<ana::is_selection_v<V>, bool>>
auto ana::dataflow::lazy<Act>::varied::filter(const std::string &name,
                                              Args &&...arguments)
    -> delayed_varied_selection_applicator_t<Args...> {

  using varied_type = delayed_varied_selection_applicator_t<Args...>;

  auto syst = varied_type(
      this->nominal().filter(name, std::forward<Args>(arguments)...));

  for (auto const &var_name : this->list_variation_names()) {
    syst.set_variation(var_name, this->variation(var_name).filter(
                                     name, std::forward<Args>(arguments)...));
  }
  return syst;
}

template <typename Act>
template <typename... Args, typename V,
          std::enable_if_t<ana::is_selection_v<V>, bool>>
auto ana::dataflow::lazy<Act>::varied::weight(const std::string &name,
                                              Args &&...arguments)
    -> delayed_varied_selection_applicator_t<Args...> {

  using varied_type = delayed_varied_selection_applicator_t<Args...>;

  auto syst = varied_type(
      this->nominal().weight(name, std::forward<Args>(arguments)...));

  for (auto const &var_name : this->list_variation_names()) {
    syst.set_variation(var_name, this->variation(var_name).weight(
                                     name, std::forward<Args>(arguments)...));
  }
  return syst;
}

template <typename Act>
template <typename... Args, typename V,
          std::enable_if_t<ana::is_selection_v<V>, bool>>
auto ana::dataflow::lazy<Act>::varied::channel(const std::string &name,
                                               Args &&...arguments)
    -> delayed_varied_selection_applicator_t<Args...> {
  using varied_type = delayed_varied_selection_applicator_t<Args...>;
  auto syst = varied_type(
      this->nominal().channel(name, std::forward<Args>(arguments)...));
  for (auto const &var_name : this->list_variation_names()) {
    syst.set_variation(var_name, this->variation(var_name).channel(
                                     name, std::forward<Args>(arguments)...));
  }
  return syst;
}

template <typename Act>
template <typename Agg, typename V,
          std::enable_if_t<ana::is_selection_v<V>, bool>>
auto ana::dataflow::lazy<Act>::varied::book(Agg &&agg) {
  return agg.book(*this);
}

template <typename Act>
template <typename... Aggs, typename V,
          std::enable_if_t<ana::is_selection_v<V>, bool>>
auto ana::dataflow::lazy<Act>::varied::book(Aggs &&...aggs) {
  return std::make_tuple((aggs.book(*this), ...));
}

template <typename Act>
template <typename V,
          std::enable_if_t<ana::is_column_v<V> ||
                               ana::aggregation::template has_output_v<V>,
                           bool>>
auto ana::dataflow::lazy<Act>::varied::operator[](
    const std::string &var_name) const -> lazy<V> {
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