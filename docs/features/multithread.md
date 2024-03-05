
Implicit multithreading of a dataflow object can be enabled by:
```cpp
qty::multithread::enable(/* optional: thread count, default: system max. */);
```
!!! warning "Thread-safety requirements"
    In order for the multithreading to be supported, developers must ensure the following:

    1. `qty::dataset::reader` must partition the dataset for parallel processing.
    2. `qty::dataset::reader` and `qty::column::reader` must access the underlying dataset in a thread-safe way.
    2. `qty::column::definition` and `qty::query::definition` must be thread-safe.