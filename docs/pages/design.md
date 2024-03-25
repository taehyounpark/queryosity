@page design Design goals
@tableofcontents

`queryosity` provides a "dataflow", or row-wise, model of structured data analysis.
Users specify operations as if they are inside for-loop over the entries of the dataset.
The key is to *specify* without executing the operations on a dataset until they are needed, i.e. create *lazy* actions.
Each lazy actions in a dataflow are only if needed in order maximize CPU usage and efficiency, which is especially relevant for analysis workflows not limited by I/O bandwidth.

This library has been purposefully developed for high-energy physics experiments; hopefully, it can be similarly useful for studying other complex phenomena.

@section design-df Why not DataFrames?

@subsection design-df-conceptual Conceptual

DataFrames can do both array-wise and row-wise operations, but the former mode is where it shines, whereas the latter is more of a fallback.
The result is a complicated API where careless mixing-and-matching of the two approaches will cost user sanity and machine performance.

Other the other hand, row-wise almost always the more intuitive way to reason with tabular datasets for humans (return to monke...).
This greatly simplifies the dataflow API, which is also distinguished by its syntax faithfully representing the actual computation graph being performed inside each entry.
In other words, Alice's analysis code can *actually* be readable to Bob, and vice versa!

@subsection design-df-technical Technical

DataFrames are optimized for a specific dataset structure in which the values in each column can be organized into a contiguous array.
Operations on these arrays of primitive data types can offer algorithmic (e.g. linear vs. binary search) as well as hardware (e.g. vectorized operations) speedups.

While this covers a wide range of data analysis workflows as evident from the widespread use of these libraries, it is not suitable for instances in which columns contain non-trivial and/or highly-nested data.
This is not so uncommon as listed above.
"Extensions" to DataFrames to support more complex column data types do exist, but they are not true panaceas as long as there is *some* data out that cannot be vectorized.

No such restrictions need exist with dataflow, as promised to be one of its main features.
It also does the best it can in terms of performance through lazy actions and multithreading.
