
#include <queryosity.hpp>
#include <queryosity/ROOT/tree.hpp>
#include <queryosity/ROOT/hist_with_toys.hpp>

using dataflow = qty::dataflow;
namespace multithread = qty::multithread;
namespace dataset = qty::dataset;
namespace column = qty::column;
namespace query = qty::query;
namespace systematic = qty::systematic;

using tree = qty::ROOT::tree;
template <unsigned int Dim, typename Prec>
using hist_with_toys = qty::ROOT::hist_with_toys<Dim, Prec>;

#include <chrono>
#include <functional>
#include <iostream>
#include <memory>
#include <sstream>

int main() {

  dataflow df(multithread::disable());

  auto one = df.define(column::constant(1));

  auto all = df.filter(one);

  auto htoys = df.get(query::output<hist_with_toys<1,float>>("htoys", std::vector<float>({0,1}),100)).fill(one,one,one,one).at(all);

  return 0;
}