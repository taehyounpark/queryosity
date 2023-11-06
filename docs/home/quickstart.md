```cpp
#include "analogical.h"

auto df = ana::dataflow( ana::multithread::enable(10) );

auto [x, w] = df.open<Json>("data.json")\
                .read<std::vector<float>, float>({"x", "w"});

auto zero = df.constant(0);
auto x0 = x[zero];

auto mask = [](std::vector<float> const& v){return v.size()};
auto masked = df.filter("mask",mask)(x).weight("weight")(w);

auto hist_x0 = df.agg<Histogram<float>>(axis::regular(10,0,1.0)).fill(x0);
auto hist_x0_result = masked.book(hist_x0).result();

std::cout << *(hist_x0.result()) << std::endl;
```