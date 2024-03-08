```cpp title="Includes"
#include "queryosity/queryosity.h"

namespace qty = queryosity;
namespace multithread = qty::multithread;
namespace dataset = qty::dataset;
namespace column = qty::column;
namespace query = qty::query;
namespace systematic = qty::systematic;
using dataflow = qty::dataflow;
```

```cpp title="Create a dataflow"
dataflow df(mulithread::enable(/*(1)!*/));
```

1. Requested number of (default: system maximum).

=== "Line-by-line"
    ```cpp title="Read columns"
    auto ds = df.load(dataset::input</*(1)!*/>(/*(2)!*/));
    auto x = ds.read(dataset::column</*(3)!*/>("x"));
    auto y = ds.read(dataset::column</*(4)!*/>("y"));
    ```

    1. `dataset::reader<Self>` implementation
    2. `dataset::reader<Self>` constructor arguments
    3. $x$ Data type
    4. $y$ Data type

=== "Less lines"
    ```cpp title="Read columns"
    auto [x, y] = df.read(
      dataset::input</*(1)!*/>(/*(2)!*/),
      dataset::column</*(3)!*/>("x")
      dataset::column</*(4)!*/>("y")
      );
    ```

    1. `dataset::reader<Self>` implementation
    2. `dataset::reader<Self>` constructor arguments
    3. $x$ Data type
    4. $y$ Data type

=== "Least lines"
    ```cpp title="Read columns"
    auto [x, y] = df.read(
      dataset::input</*(1)!*/>(/*(2)!*/),
      dataset::columns</*(3)!*/,/*(4)!*/>("x", "y")
      );
    ```

    1. `dataset::reader<Self>` implementation
    2. `dataset::reader<Self>` constructor arguments
    3. $x$ Data type
    4. $y$ Data type

```cpp title="Define columns"
auto a = ds.define(column::constant</*(1)!*/>(/*(2)!*/));
auto b = ds.define(column::expression(/*(3)!*/), /*(4)!*/);
auto c = ds.define(column::definition</*(5)!*/>(/*(6)!*/), /*(7)!*/);
```

1. Data type (optional)
2. Data value
3. Function/functor/lambda
4. Input column argument(s)
5. `column::definition<Ret(Args...)>` implementation
5. `column::definition<Ret(Args...)>` constructor arguments
7. Input column argument(s)

```cpp title="Apply selections"
auto cut = df.filter(/*(1)!*/);
auto cut_n_wgt = cut.weight(column::expression(/*(2)!*/), /*(3)!*/);
```

1. Cut decision column
2. Weight decision expression
3. Input column argument(s)

```cpp title="Perform queries"
auto q = df.make(query::plan</*(1)!*/>(/*(2)!*/)).fill(/*(3)!*/).book(/*(4)!*/);
auto q_result = q.result();
```

1. `query::definition<Out(Cols...)>` implementation
2. `query::definition<Out(Cols...)>` constructor arguments
3. Input column(s)
4. Selection

=== "Automatic"
    ```cpp title="Apply systematic variations"
    auto x = ds.vary(
      dataset::column("x_nom"),
      {"vary_x","x_var"}
      );

    auto a = df.vary(
      column::constant(/*(1)!*/),
      {"vary_a",/*(2)!*/}
      );

    auto b = df.vary(
      column::expression(/*(3)!*/),
      systematic::variation("vary_b", /*(4)!*/)
      )(/*(5)!*/);

    auto c = df.vary(
      column::definition</*(6)!*/>(/*(7)!*/),
      systematic::variation("vary_c", /*(8)!*/)
      )(/*(5)!*/);
    ```

    1. Nominal value
    2. Alternate value
    3. Nominal expression
    4. Alternate expression
    5. Input columns
    6. `column::defintion<Ret(Args...)>` implementation
    7. Nominal constructor arguments
    8. Alternate constructor arguments

=== "Manual"
    ```cpp title="Apply systematic variations"
    auto z = systematic::vary(
      systematic::nominal(/*(1)!*/), 
      systematic::variation("z_up", /*(2)!*/), 
      systematic::variation("z_dn", /*(3)!*/)
      );
    ```

    1. $z$ column
    2. $z + \Delta z$ column
    3. $z - \Delta z$ column
