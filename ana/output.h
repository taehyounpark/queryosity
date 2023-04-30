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
static void dump(Node node, Dest& dest, Args&&... args);

};

}

template <typename Sum, typename Node, typename Dest, typename... Args>
void ana::output::dump(Node node, Dest& dest, Args&&... args)
{
  // instantiate summary
  Sum summary(std::forward<Args>(args)...);

  // get counter name and selection paths
  // auto counter_name = node.from_model([](const typename Node::model_type& cntr){return cntr.get_name();});
  auto selection_paths = node.nominal().get_slots().from_model([](const typename Node::action_type& node){return node.list_selection_paths();});

  // record all results
  if constexpr( analysis_t<Node>::template is_nominal_v<Node> ) {
    // if node is nominal-only
    for (auto const& sel_path : selection_paths) {
      summary.record(sel_path, node[sel_path].result());
    }
  } else {
    for (auto const& var_name : node.list_variation_names()) {
    // if it has variations
      for (auto const& sel_path : selection_paths) {
        summary.record(var_name, sel_path, node[var_name][sel_path].result());
      }
    }
  }

  // dump all results to destination
  summary.report(dest);
}

// template <typename Sum, typename Node, typename Dest, typename... Args>
// void ana::output::dump(ana::varied<Node> syst, Dest& dest, const Args&... args)
// {

// }