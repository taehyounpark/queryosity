## Promises

- **Clear interface.** Higher-level languages have an abundance of available libraries to do intuitive and efficient data analysis. The aim is to achieve a similar level of abstraction with modern C++ syntax.
- **Customizable plugins.** Custom operations with arbitrary input(s), execution, and output(s) receive first-class treatment. From non-trivial datasets to complex computations and aggregations, there is an ABC that can be implemented.
- **Sensitivity analysis.** When changes to select column(s) need to be explored for sensitivity analysis, they have often required the dataset to be re-processed each time. With built-in handling of systematic variations, the dataset is processed *once* to retrieve all results under nominal and varied scenarios together.
- **Computational efficiency.** All operations within the dataset processing is performed at most once per-entry and only when needed. The dataset processing can be multithreaded for thread-safe operations.

## What it is *not* suited for

- Columnar analysis. `analogical` is **designed to handle non-trivial/highly-nested data types**, and the dataset processing is **inherently row-wise**. If an analysis can be expressed entirely in terms of by array(-esque) operations, e.g. [`awkward`](https://awkward-array.org/doc/main/), then those libraries with an indexing API and SIMD support will likely be cleaner and faster.