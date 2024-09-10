# Basic example

```{code} json
[
  { "x": 98.47054757472436, "v": [ 190.07135783114677, 14.80181202905574, 2.8667177988676418 ], "w": 1.0, }
]
```

1. Select entries with `v.size()` and `x > 100.0`.
2. Fill histogram with `v[0]` weighted by `w`.

```{code} cpp
#include <fstream>
#include <sstream>
#include <vector>

#include <queryosity.hpp>
#include <queryosity/hist.hpp>
#include <queryosity/json.hpp>

using dataflow = qty::dataflow;
namespace multithread = qty::multithread;
namespace dataset = qty::dataset;
namespace column = qty::column;
namespace query = qty::query;

using json = qty::json;
using h1d = qty::hist::hist<double>;
using linax = qty::hist::axis::regular;

int main() {
  dataflow df(multithread::enable(10));

  std::ifstream data("data.json");
  auto [x, v, w] = df.read(
      dataset::input<json>(data), dataset::column<double>("x"),
      dataset::column<std::vector<double>>("v"), dataset::column<double>("w"));

  auto zero = df.define(column::constant(0));
  auto v0 = v[zero];

  auto sel =
      df.weight(w)
          .filter(column::expression(
              [](std::vector<double> const &v) { return v.size(); }))(v)
          .filter(column::expression([](double x) { return x > 100.0; }))(x);

  auto h_x0_w = df.get(query::result<h1d>(linax(20, 0.0, 200.0)))
                    .fill(v0)
                    .at(sel)
                    .result();

  std::ostringstream os;
  os << *h_x0_w;
  std::cout << os.str() << std::endl;
}
```

```{code} text
histogram(regular(20, 0, 200, options=underflow | overflow))
                ┌────────────────────────────────────────────────────────────┐
[-inf,   0) 0   │                                                            │
[   0,  10) 455 │███████████████████████████████████████████████████████████ │
[  10,  20) 432 │████████████████████████████████████████████████████████    │
[  20,  30) 368 │███████████████████████████████████████████████▊            │
[  30,  40) 359 │██████████████████████████████████████████████▌             │
[  40,  50) 309 │████████████████████████████████████████▏                   │
[  50,  60) 249 │████████████████████████████████▎                           │
[  60,  70) 208 │███████████████████████████                                 │
[  70,  80) 175 │██████████████████████▊                                     │
[  80,  90) 141 │██████████████████▎                                         │
[  90, 100) 99  │████████████▉                                               │
[ 100, 110) 82  │██████████▋                                                 │
[ 110, 120) 79  │██████████▎                                                 │
[ 120, 130) 58  │███████▌                                                    │
[ 130, 140) 40  │█████▏                                                      │
[ 140, 150) 20  │██▋                                                         │
[ 150, 160) 27  │███▌                                                        │
[ 160, 170) 19  │██▌                                                         │
[ 170, 180) 20  │██▋                                                         │
[ 180, 190) 18  │██▍                                                         │
[ 190, 200) 7   │▉                                                           │
[ 200, inf) 29  │███▊                                                        │
                └────────────────────────────────────────────────────────────┘
```
