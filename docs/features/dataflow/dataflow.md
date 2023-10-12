# The DataFlow object

The dataflow object can be instantiated by:

```cpp
using dataflow = ana::dataflow;
dataflow df;
```
There are several "keyword" arguments that can be supplied to configure its processing:

| Keyword | Argument | Description |
| :--- | :--- | :--- |
| `multithread::enable(nthreads)` | `nthreads` | Enable multithreading. |
| `multithread::disable()` | | Disable multithreading. |
| `dataset::head(nrows)` | `nrows` | Only process the first number of rows. |
| `sample::weight(scale)` | `scale` | Global scale applied to all weights. |

!!! example

    Below is a dataflow object that processes the first 100 entries of a dataset with 2 concurrent threads, and applies a global weight of 0.123 to all entries.

    ```cpp
    namespace multithread = ana::multithread;
    namespace dataset = ana::dataset;
    namespace sample = ana::sample;
    dataflow df(multithread::enable(2), dataset::head(100), sample::weight(0.123));
    ```