# The `dataflow` object

The dataflow object can be instantiated by:

```cpp
using dataflow = ana::dataflow;
dataflow df;
```
There are several "keyword" arguments (can be provided in any order) available to configure the dataset processing:

| Keyword | Argument | Description |
| :--- | :--- | :--- |
| `ana::multithread::enable(nthreads)` | `nthreads` | Enable multithreading. |
| `ana::multithread::disable()` | | Disable multithreading. |
| `ana::dataset::head(nrows)` | `nrows` | Process only the first row(s) in the dataset. |
| `ana::dataset::weight(scale)` | `scale` | Apply a global scale to all weights. |

!!! example

    A dataflow object that processes the first 100 entries of a dataset with 4 concurrent threads, and applies a global weight of 0.123 to all entries.

    ```cpp
    namespace multithread = ana::multithread;
    namespace dataset = ana::dataset;
    namespace sample = ana::sample;
    dataflow df(multithread::enable(4), dataset::head(100), dataset::weight(0.123));
    ```