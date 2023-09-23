## Promises

- **Clear interface.** Higher-level languages have a myriad of libraries available to do intuitive and efficient data analysis. The syntax here aims to achieve a similar level of abstraction in its own way.
- **Interface-only.** No implementation of a data formats or aggregation output is provided out-of-the-box. Instead, the interface allows defining operations with arbitrary inputs, execution, and outputs as needed.
- **Sensitivity analysis.** Often times, changes to an analysis need to be explored for sensitivity analysis. How many times has this required the dataset to be re-processed? With built-in handling of systematic variations, changes can their impacts retrieved all together.
- **Computational efficiency.** All operations within the dataset processing is performed at most once per-entry, only when needed. All systematic variations are processed at once. The dataset processing is multithreaded for thread-safe plugins.

## What it is *not* suited for

- Flat columns. This library is **intended to handle arbitrary, non-trivial, highly-nested data**, and the dataset processing is **inherently row-wise**. So if an analysis can be expressed entirely in terms of by e.g. columnar/array-like operations, then another library with an indexing API and SIMD support will be cleaner and faster 90% of the time.