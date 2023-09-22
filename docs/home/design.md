## Promises

- **Clear syntax.** Higher-level languages have myriad of libraries available to do columnar data analysis intuitively, e.g. "DataFrame". The syntax used here aims to achieve a similar level of abstraction in its own way, referred to as "DataFlow" here.
- **Interface-only.** No implementation of a data formats or aggregation output is provided out-of-the-box. Instead, the interface allows defining operations with arbitrary inputs, execution, and outputs as needed.
- **Non-proliferative workflow.** Often times, small changes to an analysis need to be explored. How many times has CTRL+C/V been used to copy an entire analysis, made minute changes, and re-process the dataset? With built-in handling of "systematic variations", such changes can be performed and retrieved simultaneously.
- **Computational efficiency.** All operations within the dataset processing is performed at most once per-entry, only when needed. All systematic variations are processed at once. The dataset processing is multithreaded for thread-safe plugins.

## What it is *not* for

- Flat column types. This library is **intended to handle arbitrary, non-trivial, highly-nested column data types**, and as such the dataset processing is **inherently row-wise**. So if all of your analysis needs can be satisfied by e.g. columnar/array-like operations, then another library with an indexing API and SIMD support will be cleaner and faster 90% of the time.