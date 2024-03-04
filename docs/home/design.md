# Design goals

`queryosity` is aims to achieve the following:

- **Clear interface.** Use a clear abstraction layer with modern C++ syntax to describe even the most complex analyses.
- **Customizable actions.** Support for custom datasets and queries, as well as arbitrary computations in-between.
- **Sensitivity analysis.** Systematic variations within an analysis are automatically propagated and simultaneously processed.
- **Computational efficiency.** Actions are performed for an entry only when required. Dataset traversal is multithreaded.

It does *not* strive to be well-suited for:

- Array-wise, i.e. columnar, analysis. Being **designed to handle arbitrary data types**, the dataset traversal is **inherently row-wise**. If an analysis consists mainly of bulk operations on arrays, then libraries with an index-based API (and SIMD support) will be better-suited (and faster).