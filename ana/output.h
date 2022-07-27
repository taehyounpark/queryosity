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

struct output
{

template <typename Dumper, typename Node, typename Dest, typename... Args>
static void dump(Node bkr, Dest& dest, const Args&... args);

};

}

template <typename Dumper, typename Node, typename Dest, typename... Args>
void ana::output::dump(Node node, Dest& dest, const Args&... args)
{
  // instantiate dmpr
  Dumper dmpr(dest, args...);

  // access main booker
  auto selection_paths = node.check([](const typename Node::model_type& mod){return mod.list_selection_paths();});

  // get list of counters
  using result_type = decltype(node[""].result());
  std::vector<std::pair<std::shared_ptr<ana::counter>,result_type>> results;
  for (const auto& sel_path : selection_paths) {
    results.emplace_back(node[sel_path].model(), node[sel_path].result());
  }

  // run dmpr on results
  dmpr.dump(results);
}
