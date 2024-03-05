```cpp
#include "queryosity.h"

#include "queryosity/json.h"
#include "queryosity/hist.h"

namespace qty = queryosity;

auto df = qty::dataflow( qty::multithread::enable(10) );

auto [ds, x, w] = df.read( qty::dataset::input<qty::json>("data.json"), 
                           qty::dataset::columns<std::vector<float>, float>("x", "w") );

auto zero = df.define( qty::column::constant(0) );
auto x0 = x[zero];

auto masked_n_weighted = df.filter(
  qty::column::expression(
    [](std::vector<float> const& v){return v.size()}
    ), x
  ).weight(w);

auto hist = df.get( 
  qty::query::output<qty::hist<float>>( 
    qty::axis::linear(100,0.0,1.0)
    ) 
  ).fill(x0).book(masked_n_weighted);

std::ostringstream os;
os << *(hist.result());
std::cout << os.str() << std::endl;
```