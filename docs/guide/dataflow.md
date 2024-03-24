# Dataflow

```cpp
// (1)
dataflow df;

// (2)
dataflow df(multithread::enable(4), dataset::weight(0.123), dataset::head(100));
```

| Keyword Argument | Description |
| :--- | :--- |
| `multithread::enable(nthreads)` | Enable multithreading up to `nthreads`. |
| `multithread::disable()` | Disable multithreading. |
| `dataset::head(nrows)` | Process only the first `nrows` of the dataset. |
| `dataset::weight(scale)` | Apply a global `scale` to all weights. |

In order for the multithreading to be supported, developers must ensure the following:

1. `dataset::reader` must partition the dataset for parallel processing.
2. `dataset::reader` and `column::reader` must access the underlying dataset in a thread-safe way.
2. `column::definition` and `query::definition` must be thread-safe.