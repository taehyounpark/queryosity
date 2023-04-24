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
  // instantiate reporter
  Reporter reporter(args...);

  // get counter name and selection paths
  // auto counter_name = node.check([](const typename Node::model_type& cntr){return cntr.get_name();});
  auto selection_paths = node.check([](const typename Node::model_type& bkr){return bkr.list_selection_paths();});

  // record all results
  using result_type = decltype(node["selection"].result());
  for (const auto& sel_path : selection_paths) {
    reporter.record(sel_path, node[sel_path].result());
  }

  // report results to dest
  reporter.report(dest);
}
