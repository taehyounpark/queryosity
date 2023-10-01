#pragma once

#include <utility>

namespace ana {

namespace output {

template <typename Summary, typename Bookkeeper, typename Destination,
          typename... Args>
void dump(Bookkeeper const &node, Destination &&dest, Args &&...args);

}

} // namespace ana

#include "dataflow.h"

template <typename Summary, typename Bookkeeper, typename Destination,
          typename... Args>
void ana::output::dump(Bookkeeper const &node, Destination &&dest,
                       Args &&...args) {
  // instantiate summary
  Summary summary(std::forward<Args>(args)...);

  // get selection paths
  auto selection_paths = node.nominal().get_model_value(
      [](typename Bookkeeper::nominal_type const &node) {
        return node.list_selection_paths();
      });

  // record all results
  if constexpr (dataflow::template is_nominal_v<Bookkeeper>) {
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
  summary.output(std::forward<Destination>(dest));
}