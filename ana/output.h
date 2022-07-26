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

template <typename Nd, typename Out, typename F>
static void dump(Nd bkr, Out& out, F&& dumper);

};

}

template <typename Nd, typename Out, typename F>
void ana::output::dump(Nd node, Out& out, F&& dumper)
{
  // access main booker
  auto bkr = node.model();

  // get list of counters
  using result_type = decltype(bkr->list_counters()[0]->result());
  std::vector<std::pair<std::shared_ptr<ana::counter>,result_type>> results;
  for (const auto& cnt : bkr->list_counters()) {
    results.emplace_back(cnt, cnt->result());
  }

  // run dumper on results
  dumper(results, out);
}
