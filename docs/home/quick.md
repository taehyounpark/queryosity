```cpp
#include "analogical.h"

#include "Json.h"
#include "Histogram.h"

auto df = ana::dataflow( ana::multithread::enable(10) );

auto [x, w] = df.open<Json>("data.json")
                .read<std::vector<float>, float>({"x", "w"});

auto zero = df.constant(0);
auto x0 = x[zero];

auto mask = [](std::vector<float> const& v){return v.size()};
auto masked = df.filter("mask",mask)(x).weight("weight")(w);

auto hist; = df.agg<Histogram<float>>(LinearAxis(100,0.0,1.0));
auto hist_x0_masked = hist.fill(x0).book(masked).result();

std::ostringstream os; os << *(hist_x0_masked);
std::cout << os.str() << std::endl;
```