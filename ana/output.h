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
template <typename Reporter, typename Node, typename Dest, typename... Args>
static void dump(Node bkr, Dest& dest, const Args&... args);

};

}

template <typename Reporter, typename Node, typename Dest, typename... Args>
void ana::output::dump(Node node, Dest& dest, const Args&... args)
{
  // instantiate summary
  Reporter summary(args...);

  // get counter name and selection paths
  // auto counter_name = node.from_model([](const typename Node::model_type& cntr){return cntr.get_name();});
  auto selection_paths = node.get_action().from_model([](const typename Node::action_type& bkr){return bkr.list_selection_paths();});

  // record all results
  // using result_type = decltype(node["path"].result());
  for (const auto& sel_path : selection_paths) {
    summary.record(sel_path, node[sel_path].result());
  }

  // report results to dest
  summary.report(dest);
}

// template <typename Reporter, typename Node, typename Dest, typename... Args>
// void ana::output::dump(ana::varied<Node> syst, Dest& dest, const Args&... args)
// {

// }