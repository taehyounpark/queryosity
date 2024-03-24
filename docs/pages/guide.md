@page guide User guide
@tableofcontents

@section guide-cheatsheet Cheat sheet

```cpp
#include "queryosity/queryosity.h"

namespace qty = queryosity;
namespace multithread = qty::multithread;
namespace dataset = qty::dataset;
namespace column = qty::column;
namespace query = qty::query;
namespace systematic = qty::systematic;
using dataflow = qty::dataflow;
```

```cpp
dataflow df(mulithread::enable(nthreads));
```

```cpp
// (1)
auto ds = df.load(dataset::input</* DS */>(/* input_arguments */));
auto x = ds.read(dataset::column<>("x"));
auto y = ds.read(dataset::column<>("y"));

// (2)
auto [x, y] = df.read(
  dataset::input</* DS */>(/* input_arguments */),
  dataset::column<>("x"),
  dataset::column<>("y")
  );

// (3)
auto [x, y] = df.read(
  dataset::input</* DS */>(/* input_arguments */),
  dataset::columns<>("x", "y")
  );
```

```cpp
auto a = ds.define(column::constant<>());
auto b = ds.define(column::expression(), );
auto c = ds.define(column::definition<>(), );
```

```cpp
auto cut = df.filter();
auto cut_n_wgt = cut.weight(column::expression(), );
```

```cpp
auto q = df.make(query::plan<>()).fill().book();
auto q_result = q.result();
```

```cpp
// (1)
auto x = ds.vary(
  dataset::column("x_nom"),
  {"vary_x","x_var"}
  );

auto a = df.vary(
  column::constant(),
  {"vary_a",}
  );

auto b = df.vary(
  column::expression(),
  systematic::variation("vary_b", )
  )();

auto c = df.vary(
  column::definition<>(),
  systematic::variation("vary_c", /*(8)!*/)
  )();

// (2)
auto z_nom = df.define();
auto z_up = df.define();
auto z_dn = df.define();

auto z = systematic::vary(
  systematic::nominal(), 
  systematic::variation("z_up", ), 
  systematic::variation("z_dn", )
  );

```

@section guide-dataflow Dataflow

```cpp
dataflow df;
```

```cpp
dataflow df(multithread::enable(4), dataset::weight(0.123), dataset::head(100));
```

In order for the multithreading to be supported, developers must ensure the following:

1. `dataset::reader` must partition the dataset for parallel processing.
2. `dataset::reader` and `column::reader` must access the underlying dataset in a thread-safe way.
3. `column::definition` and `query::definition` must be thread-safe.

| Keyword Argument | Description |
| :--- | :--- |
| `multithread::enable(nthreads)` | Enable multithreading up to `nthreads`. |
| `multithread::disable()` | Disable multithreading. |
| `dataset::head(nrows)` | Process only the first `nrows` of the dataset. |
| `dataset::weight(scale)` | Apply a global `scale` to all weights. |