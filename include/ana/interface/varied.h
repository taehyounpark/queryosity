#pragma once

#include <set>
#include <type_traits>
#include <unordered_map>
#include <utility>

#include "lazy.h"

#define DECLARE_VARIED_BINARY_OP(op_symbol)                                    \
  template <typename Arg>                                                      \
  auto operator op_symbol(Arg &&b) const->typename lazy<                       \
      typename decltype(std::declval<lazy<Act>>().operator op_symbol(          \
          std::forward<Arg>(b).get_nominal()))::action_type>::varied;
#define DEFINE_VARIED_BINARY_OP(op_symbol)                                     \
  template <typename T>                                                        \
  template <typename Act>                                                      \
  template <typename Arg>                                                      \
  auto ana::dataflow<T>::lazy<Act>::varied::operator op_symbol(Arg &&b)        \
      const->typename lazy<                                                    \
          typename decltype(std::declval<lazy<Act>>().operator op_symbol(      \
              std::forward<Arg>(b).get_nominal()))::action_type>::varied {     \
    auto syst = typename lazy<                                                 \
        typename decltype(std::declval<lazy<Act>>().operator op_symbol(        \
            std::forward<Arg>(b).get_nominal()))::action_type>::               \
        varied(this->get_nominal().operator op_symbol(                         \
            std::forward<Arg>(b).get_nominal()));                              \
    for (auto const &var_name :                                                \
         list_all_variation_names(*this, std::forward<Arg>(b))) {              \
      syst.set_variation(var_name,                                             \
                         get_variation(var_name).operator op_symbol(           \
                             std::forward<Arg>(b).get_variation(var_name)));   \
    }                                                                          \
    return syst;                                                               \
  }
#define DECLARE_VARIED_UNARY_OP(op_symbol)                                     \
  template <typename V = Act,                                                  \
            std::enable_if_t<ana::is_column_v<V>, bool> = false>               \
  auto operator op_symbol() const->typename lazy<                              \
      typename decltype(std::declval<lazy<V>>().                               \
                        operator op_symbol())::action_type>::varied;
#define DEFINE_VARIED_UNARY_OP(op_name, op_symbol)                             \
  template <typename T>                                                        \
  template <typename Act>                                                      \
  template <typename V, std::enable_if_t<ana::is_column_v<V>, bool>>           \
  auto ana::dataflow<T>::lazy<Act>::varied::operator op_symbol() const->       \
      typename lazy<                                                           \
          typename decltype(std::declval<lazy<V>>().                           \
                            operator op_symbol())::action_type>::varied {      \
    auto syst =                                                                \
        typename lazy<typename decltype(std::declval<lazy<V>>().               \
                                        operator op_symbol())::action_type>::  \
            varied(this->get_nominal().operator op_symbol());                  \
    for (auto const &var_name : list_all_variation_names(*this)) {             \
      syst.set_variation(var_name,                                             \
                         get_variation(var_name).operator op_symbol());        \
    }                                                                          \
    return syst;                                                               \
  }

namespace ana {

/**
 * @brief Variations of a lazy action to be performed in an dataflow.
 * @tparam T Input dataset type
 * @tparam U Actions to be performed lazily.
 * @details A `varied` node can be treated identical to a `lazy` one, except
 * that it contains multiple variations of the action as dictated by the
 * analyzer that propagate through the rest of the analysis.
 */
template <typename T>
template <typename Act>
class dataflow<T>::lazy<Act>::varied : public node<lazy<Act>> {

  // public:
  //   using dataflow_type = typename lazy<Act>::dataflow_type;
  //   using dataset_type = typename lazy<Act>::dataset_type;
  //   using action_type = typename lazy<Act>::action_type;

public:
  varied(lazy<Act> const &nom);
  ~varied() = default;

  virtual void set_variation(const std::string &var_name,
                             const lazy &var) override;

  virtual lazy get_nominal() const override;
  virtual lazy get_variation(const std::string &var_name) const override;

  virtual bool has_variation(const std::string &var_name) const override;
  virtual std::set<std::string> list_variation_names() const override;

  DECLARE_VARIED_UNARY_OP(-)
  DECLARE_VARIED_BINARY_OP(+)

protected:
  lazy<Act> m_nom;
  std::unordered_map<std::string, lazy<Act>> m_var_lookup;
  std::set<std::string> m_var_names;
};

} // namespace ana

template <typename T>
template <typename Act>
ana::dataflow<T>::lazy<Act>::varied::varied(lazy<Act> const &nom)
    : node<lazy<Act>>::node(nom), m_nom(nom) {}

template <typename T>
template <typename Act>
void ana::dataflow<T>::lazy<Act>::varied::set_variation(
    const std::string &var_name, lazy const &var) {
  m_var_lookup.insert(std::make_pair(var_name, var));
  m_var_names.insert(var_name);
}

template <typename T>
template <typename Act>
auto ana::dataflow<T>::lazy<Act>::varied::get_nominal() const -> lazy {
  return m_nom;
}

template <typename T>
template <typename Act>
auto ana::dataflow<T>::lazy<Act>::varied::get_variation(
    const std::string &var_name) const -> lazy {
  return (this->has_variation(var_name) ? m_var_lookup.at(var_name) : m_nom);
}

template <typename T>
template <typename Act>
bool ana::dataflow<T>::lazy<Act>::varied::has_variation(
    const std::string &var_name) const {
  return m_var_lookup.find(var_name) != m_var_lookup.end();
}

template <typename T>
template <typename Act>
std::set<std::string>
ana::dataflow<T>::lazy<Act>::varied::list_variation_names() const {
  return m_var_names;
}

// template <typename T>
// template <typename Act>
// template <typename Sel, typename V,
//           std::enable_if_t<ana::is_selection_v<V>, bool>>
// auto ana::dataflow<T>::lazy<Act>::varied::filter(const std::string &name)
//     -> varied<selection::trivial_applicator_type> {
//   varied<selection::trivial_applicator_type> syst(
//       this->get_nominal().template filter<Sel>(name));
//   for (auto const &var_name : this->list_variation_names()) {
//     syst.set_variation(var_name,
//                        get_variation(var_name).template filter<Sel>(name));
//   }
//   return syst;
// }

// template <typename T>
// template <typename Act>
// template <typename Sel, typename V,
//           std::enable_if_t<ana::is_selection_v<V>, bool>>
// auto ana::dataflow<T>::lazy<Act>::varied::channel(const std::string &name)
//     -> varied<selection::trivial_applicator_type> {
//   varied<selection::trivial_applicator_type> syst(
//       this->get_nominal().template channel<Sel>(name));
//   for (auto const &var_name : this->list_variation_names()) {
//     syst.set_variation(var_name,
//                        get_variation(var_name).template channel<Sel>(name));
//   }
//   return syst;
// }

// template <typename T>
// template <typename Act>
// template <typename Sel, typename Lmbd, typename V,
//           std::enable_if_t<ana::is_selection_v<V>, bool>>
// auto ana::dataflow<T>::lazy<Act>::varied::filter(const std::string &name,
// Lmbd &&lmbd)
//     -> varied<selection::template custom_applicator_t<Lmbd>> {
//   varied<selection::template custom_applicator_t<Lmbd>> syst(
//       this->get_nominal().template filter<Sel>(name,
//       std::forward<Lmbd>(lmbd)));
//   for (auto const &var_name : this->list_variation_names()) {
//     syst.set_variation(var_name, get_variation(var_name).template
//     filter<Sel>(
//                                      name, std::forward<Lmbd>(lmbd)));
//   }
//   return syst;
// }

// template <typename T>
// template <typename Act>
// template <typename Sel, typename Lmbd, typename V,
//           std::enable_if_t<ana::is_selection_v<V>, bool>>
// auto ana::dataflow<T>::lazy<Act>::varied::channel(const std::string &name,
//                                             Lmbd &&lmbd)
//     -> varied<selection::template custom_applicator_t<Lmbd>> {
//   varied<selection::template custom_applicator_t<Lmbd>> syst(
//       this->get_nominal().template channel<Sel>(name,
//                                                 std::forward<Lmbd>(lmbd)));
//   for (auto const &var_name : this->list_variation_names()) {
//     syst.set_variation(var_name, get_variation(var_name).template
//     channel<Sel>(
//                                      name, std::forward<Lmbd>(lmbd)));
//   }
//   return syst;
// }

// template <typename T>
// template <typename Act>
// template <typename Node, typename V,
//           std::enable_if_t<ana::is_selection_v<V>, bool>>
// auto ana::dataflow<T>::lazy<Act>::varied::operator&&(const Node &b) const
//     -> varied<V> {
//   auto syst = varied<typename decltype(std::declval<lazy<Act>>().operator&&(
//       b.get_nominal()))::action_type>(
//       this->get_nominal().operator&&(b.get_nominal()));
//   for (auto const &var_name : list_all_variation_names(*this, b)) {
//     syst.set_variation(var_name, this->get_variation(var_name).operator&&(
//                                      b.get_variation(var_name)));
//   }
//   return syst;
// }

// template <typename T>
// template <typename Act>
// template <typename Node, typename V,
//           std::enable_if_t<ana::is_selection_v<V>, bool>>
// auto ana::dataflow<T>::lazy<Act>::varied::operator||(const Node &b) const
//     -> varied<V> {
//   auto syst = varied<typename decltype(std::declval<lazy<Act>>().operator||(
//       b.get_nominal()))::action_type>(
//       this->get_nominal().operator||(b.get_nominal()));
//   for (auto const &var_name : list_all_variation_names(*this, b)) {
//     syst.set_variation(var_name, this->get_variation(var_name).operator||(
//                                      b.get_variation(var_name)));
//   }
//   return syst;
// }

DEFINE_VARIED_UNARY_OP(minus, -)
// DEFINE_VARIED_UNARY_OP(logical_not, !)

DEFINE_VARIED_BINARY_OP(+)
// DEFINE_VARIED_BINARY_OP(-)
// DEFINE_VARIED_BINARY_OP(*)
// DEFINE_VARIED_BINARY_OP(/)
// DEFINE_VARIED_BINARY_OP(<)
// DEFINE_VARIED_BINARY_OP(>)
// DEFINE_VARIED_BINARY_OP(<=)
// DEFINE_VARIED_BINARY_OP(>=)
// DEFINE_VARIED_BINARY_OP(==)
// DEFINE_VARIED_BINARY_OP(&&)
// DEFINE_VARIED_BINARY_OP(||)
// DEFINE_VARIED_BINARY_OP([])
