#pragma once

#include <set>
#include <string>
#include <type_traits>

namespace queryosity {

template <typename T> class lazy;

namespace systematic {

template <typename... Nodes>
auto get_variation_names(Nodes const &...nodes) -> std::set<std::string>;

template <typename Node> class resolver;

} // namespace systematic

} // namespace queryosity

template <typename... Nodes>
auto queryosity::systematic::get_variation_names(Nodes const &...nodes)
    -> std::set<std::string> {
  std::set<std::string> variation_names;
  (variation_names.merge(nodes.get_variation_names()), ...);
  return variation_names;
}