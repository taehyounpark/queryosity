## Promises

- **Clear interface.** Specify operations with a clear, high-level abstraction interface using modern C++ syntax.
- **Customizable plugins.** Operations with arbitrary inputs, execution, and outputs receive first-class treatment: from custom datasets and columns to complex aggregations, there is a customizable ABC.
- **Sensitivity analysis.** With built-in handling of systematic variations, changes to an operation are automatically propagated and all results under the original and varied scenarios are obtained simultaneously.
- **Computational efficiency.** Dataset operations are (1) multithreaded, and (2) performed for an entry only if needed.

## What it is *not* suited for

- Columnar analysis. `analogical` is **designed to handle non-trivial/highly-nested data types**; as such, the dataset processing is **inherently row-wise**. If an analysis is more intuitively expressed in terms of array operations, then libraries with an index-based API (and SIMD support) will be better suited (and faster).