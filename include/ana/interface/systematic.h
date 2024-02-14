#pragma once

/** @file */

#include <set>
#include <string>
#include <type_traits>

namespace ana {

namespace systematic {

template <typename... Nodes>
auto list_all_variation_names(Nodes const &...nodes) -> std::set<std::string>;

template <typename Node> class resolver;

template <typename... Args> class variation;

class mode {
public:
  mode() : m_is_nominal(true), m_variation_name("") {}
  ~mode() = default;

  void set_mode(bool is_nominal, const std::string &var_name) {
    m_is_nominal = is_nominal;
    m_variation_name = var_name;
  }

  bool is_nominal() const { return m_is_nominal; }
  std::string variation_name() const {
    return m_is_nominal ? "" : m_variation_name;
  }

protected:
  bool m_is_nominal;
  std::string m_variation_name;
};

} // namespace systematic

} // namespace ana

template <typename... Nodes>
auto ana::systematic::list_all_variation_names(Nodes const &...nodes)
    -> std::set<std::string> {
  std::set<std::string> variation_names;
  (variation_names.merge(nodes.list_variation_names()), ...);
  return variation_names;
}