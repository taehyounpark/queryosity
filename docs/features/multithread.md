
Implicit multithreading of a dataflow object can be enabled by:
```cpp
ana::multithread::enable(/* optional: thread count, default: system max. */);
```
!!! warning "Thread-safety requirements"
    In order for the multithreading to be valid for, analyzers must ensure the following:

    1. `ana::dataset::input` must define a way of partitioning the dataset for parallel processing.
    2. `ana::dataset::player` and `ana::dataset::column` must access the underlying data in a thread-safe way.
    2. `ana::column::definition` and `ana::counter::logic` implementations must be thread-safe.