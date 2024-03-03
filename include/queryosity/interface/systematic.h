#pragma once

/** @file */

#include <set>
#include <string>
#include <type_traits>

namespace queryosity {

namespace systematic {

template <typename... Nodes>
auto list_all_variation_names(Nodes const &...nodes) -> std::set<std::string>;

template <typename Node> class resolver;

template <typename... Args> class variation;

class mode {
public:
  mode() : m_variation_name("") {}
  ~mode() = default;

  void set_variation_name(const std::string &var_name) {
    m_variation_name = var_name;
  }

  bool is_nominal() const { return m_variation_name.empty(); }
  std::string variation_name() const { return m_variation_name; }

protected:
  std::string m_variation_name;
};

} // namespace systematic

} // namespace queryosity

template <typename... Nodes>
auto queryosity::systematic::list_all_variation_names(Nodes const &...nodes)
    -> std::set<std::string> {
  std::set<std::string> variation_names;
  (variation_names.merge(nodes.list_variation_names()), ...);
  return variation_names;
}