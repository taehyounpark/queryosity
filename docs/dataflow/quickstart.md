```cpp
#include "queryosity.h"

#include "queryosity/json.h"
#include "queryosity/hist.h"

namespace qty = queryosity;

auto df = qty::dataflow( qty::multithread::enable(10) );

auto [x, w] = df.read( 
  qty::dataset::input<qty::json>("data.json"), 
  qty::dataset::columns<std::vector<double>, double>("x", "w") 
  );

auto zero = df.define( qty::column::constant(0) );
auto x0 = x[zero];

auto x0_exists_n_weight = df.filter(
  qty::column::expression([](std::vector<double> const& v){return v.size()}), x
  ).weight(w);

auto hist_x0_weighted = df.make( 
  qty::query::plan<qty::hist<float>>( qty::axis::linear(100,0.0,1.0) ) 
  ).fill(x0).book(x0_exists_n_weight).result();
```