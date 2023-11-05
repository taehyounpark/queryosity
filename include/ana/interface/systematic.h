#pragma once

/** @file */

#include <set>
#include <string>
#include <type_traits>

#include "dataflow.h"

namespace ana {

namespace systematic {

template <typename Node> class resolver;

template <typename... Args> class variation;

template <typename... Nodes>
auto list_all_variation_names(Nodes const &...nodes) -> std::set<std::string>;

} // namespace systematic

} // namespace ana

template <typename... Nodes>
auto ana::systematic::list_all_variation_names(Nodes const &...nodes)
    -> std::set<std::string> {
  std::set<std::string> variation_names;
  (variation_names.merge(nodes.list_variation_names()), ...);
  return variation_names;
}