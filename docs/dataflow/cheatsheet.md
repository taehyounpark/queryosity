```cpp title="Includes"
#include "queryosity/queryosity.h"

namespace qty = queryosity;
namespace multithread = qty::multithread;
namespace dataset = qty::dataset;
namespace column = qty::column;
namespace query = qty::query;
using dataflow = qty::dataflow;
```

```cpp title="Create a dataflow"
dataflow df(mulithread::enable(/*(1)!*/));
```

1. Requested number of (default: system maximum).

```cpp title="Load dataset"
auto ds = df.load(dataset::input</*(1)!*/>(/*(2)!*/));
```

1. `dataset::reader<Self>` implementation
1. `dataset::reader<Self>` constructor arguments

```cpp title="Read columns"
auto x = ds.read(dataset::column</*(1)!*/>(/*(2)!*/));
```

1. Data type
2. Column name

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

```cpp title="Apply systematic variations"
auto z = df.vary(
  systematic::nominal(/*(1)!*/), 
  systematic::variation("z_up", /*(2)!*/), 
  systematic::variation("z_dn", /*(3)!*/)
  /*(4)!*/
  );
```

1. $z$ column
2. $z + \Delta z$ column
3. $z - \Delta z$ column
4. Any other number of variations