# The DataFlow object

Instantiate a dataflow object by simply calling:
```cpp
using dataflow = ana::dataflow;
dataflow df;
```
There are several "keyword" arguments that can be supplied to configure its processing:

| Keyword | Argument | Description |
| :--- | :--- | :--- |
| `multithread::enable(nthreads)` | `nthreads` | Enable multithreading. |
| `multithread::disable()` | ---| Disable multithreading. |
| `dataset::head(nrows)` | `nrows` | Only process the first number of rows. |
| `sample::weight(scale)` | `scale` | Global scale applied to all weights. |

```cpp
namespace multithread = ana::multithread;
namespace dataset = ana::dataset;
namespace sample = ana::sample;
dataflow df(multithread::enable(), dataset::head(100), sample::weight(0.123));
```