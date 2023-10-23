## Promises

- **Clear interface.** Higher-level languages have an abundance of available libraries to do intuitive and efficient data analysis. An interface with a similar level of abstraction with modern C++ syntax.
- **Customizable plugins.** Arbitrary operations with custom input(s), execution, and output(s) receive first-class treatment. From non-trivial datasets to complex computations and aggregations, there is an ABC available for implementation.
- **Sensitivity analysis.** With built-in handling of systematic variations, changes in operations can be processed *once* to retrieve all results under nominal and varied scenarios simultaneously.
- **Computational efficiency.** Operations within the dataset processing are performed at most once per-entry and only when needed. If enabled, the processing is multithreaded.

## What it is *not* suited for

- Columnar analysis. `analogical` is **designed to handle non-trivial/highly-nested data types**, and the dataset processing is **inherently row-wise**. If an analysis can be expressed entirely in terms of by array operations, then libraries with an index-based API (and SIMD support) will be cleaner (and faster).