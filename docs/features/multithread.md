
Implicit multithreading of a dataflow object can be enabled by:
```cpp
queryosity::multithread::enable(/* optional: thread count, default: system max. */);
```
!!! warning "Thread-safety requirements"
    In order for the multithreading to be valid for, analyzers must ensure the following:

    1. `queryosity::dataset::input` must define a way of partitioning the dataset for parallel processing.
    2. `queryosity::dataset::player` and `queryosity::dataset::column` must access the underlying data in a thread-safe way.
    2. `queryosity::column::definition` and `queryosity::query::definition` implementations must be thread-safe.