# Design goals

- **Clear interface.** Use a clear high-level abstraction layer with modern C++ syntax to specify even the most complex operations.
- **Customizable operations.** Support for inputs and outputs of any type and arbitrary execution, such as custom dataset formats, column definitions, and counter algorithms.
- **Sensitivity analysis.** Systematic variations of an analysis are automatically propagated and simultaneously processed within one dataset traversal.
- **Computational efficiency.** Dataset operations are performed for an entry only if needed. Dataset traversal is multithreaded.

# Non-goals

- Columnar analysis. Being **designed to handle arbitrary data types**, the dataset traversal is **inherently row-wise**. If an analysis can be expressed as array operations, then libraries with an index-based API (and SIMD support) will be better-suited (and faster).
- Best-ever speed. One can expect **overall performance** to be **comparable to other frameworks**; some benchmarks are available [here](https://github.com/iris-hep/adl-benchmarks-index). If there is a (not array-wise) workflow that is significantly lacking in speed, please submit an issue on GitHub.