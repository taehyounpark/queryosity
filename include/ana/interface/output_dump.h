#pragma once

#include <utility>

namespace ana {

namespace output {

template <typename Sum, typename Node, typename Dest, typename... Args>
void dump(Node const &node, Dest &&dest, Args &&...args);

}

} // namespace ana

#include "dataflow.h"

template <typename Sum, typename Node, typename Dest, typename... Args>
void ana::output::dump(Node const &node, Dest &&dest, Args &&...args) {
  // instantiate summary
  Sum summary(std::forward<Args>(args)...);

  // get selection paths
  auto selection_paths = node.nominal().get_model_value(
      [](typename Node::nominal_type const &node) {
        return node.list_selection_paths();
      });

  // record all results
  if constexpr (dataflow_t<Node>::template is_nominal_v<Node>) {
    // if node is nominal-only
    for (auto const &selection_path : selection_paths) {
      summary.record(selection_path, node[selection_path].result());
    }
  } else {
    // if it has variations
    for (auto const &selection_path : selection_paths) {
      summary.record(selection_path, node.nominal()[selection_path].result());
    }
    for (auto const &var_name : list_all_variation_names(node)) {
      for (auto const &selection_path : selection_paths) {
        summary.record(var_name, selection_path,
                       node[var_name][selection_path].result());
      }
    }
  }
  // dump all results to destination
  summary.output(std::forward<Dest>(dest));
}