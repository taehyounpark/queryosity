
Implicit multithreading of a dataflow object can be enabled by:
```cpp
ana::multithread::enable(/* 10 */);
```
!!! warning "Thread-safety requirements"
    In order for the multithreading to be valid for, analyzers must ensure the following:

    1. `ana::dataset::input` must implement a valid logic of partitioning the dataset for parallel processing.
    2. `ana::dataset::reader` and `ana::column::reader` must access the underlying data in a thread-safe way.
    2. `ana::column::definition` and `ana::aggregation::logic` implementations must be thread-safe.