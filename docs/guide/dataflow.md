# Dataflow

```cpp
#include "queryosity.h"

using dataflow = qty::dataflow;
namespace multithread = qty::multithread;
namespace dataset = qty::dataset;
namespace column = qty::column;
namespace query = qty::query;
namespace systematic = qty::systematic;

int main() {

  dataflow df;

  // your analysis here...

}
```

The dataflow accepts (up to three) optional keyword arguments options to configure the dataset processing:

| Option | Description | Default |
| :--- | :--- | :--- |
| `multithread::enable(nthreads)` | Enable multithreading up to `nthreads`. | `-1` (system maximum) |
| `multithread::disable()` | Disable multithreading. | |
| `dataset::weight(scale)` | Apply a global `scale` to all weights. | `1.0` |
| `dataset::head(nrows)` | Process the first `nrows` of the dataset. | `-1` (all entries) |

:::{example}
```cpp
dataflow df(multithread::enable(10), dataset::weight(1.234), dataset::head(100));
```
:::
