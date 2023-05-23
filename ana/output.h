#pragma once

#include <vector>
#include <memory>
#include <string>
#include <type_traits>
#include <functional>

#include "ana/analysis.h"
#include "ana/counter.h"

namespace ana
{

class output
{

public:
template <typename Sum, typename Node, typename Dest, typename... Args>
static void dump(Node const& node, Dest&& dest, Args&&... args);

};

}

template <typename Sum, typename Node, typename Dest, typename... Args>
void ana::output::dump(Node const& node, Dest&& dest, Args&&... args)
{
  // instantiate summary
  Sum summary(std::forward<Args>(args)...);

  // get selection paths
  auto selection_paths = node.get_nominal().get_model_value([](typename Node::action_type const& node){return node.list_selection_paths();});

  // record all results
  if constexpr( analysis_t<Node>::template is_nominal_v<Node> ) {
    // if node is nominal-only
    for (auto const& sel_path : selection_paths) {
      summary.record(sel_path, node[sel_path].get_result());
    }
  } else {
    // if it has variations
    for (auto const& sel_path : selection_paths) {
      summary.record(sel_path, node.get_nominal()[sel_path].get_result());
    }
    for (auto const& var_name : list_all_variation_names(node)) {
      for (auto const& sel_path : selection_paths) {
        summary.record(var_name, sel_path, node[var_name][sel_path].get_result());
      }
    }
  }

  // dump all results to destination
  summary.output(std::forward<Dest>(dest));
}

// template <typename Sum, typename Node, typename Dest, typename... Args>
// void ana::output::dump(ana::varied<Node> syst, Dest& dest, const Args&... args)
// {

// }