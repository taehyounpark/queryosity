`queryosity` provides a "dataflow" model of structured data analysis.
Possible use cases include: 

- Data analysis of complex phenomena, e.g. high-energy physics experiments (the author's primary purpose).
- Data processing pipelines to transform complicated data structures into simpler ones.

Analyzers interact with a row-wise interface; in other words, specify operations as if they were inside the entry-loop of the dataset.
The keyword here is to *specify* without executing operations that need to be performed on a dataset until they are needed, which are called *lazy* actions.
All actions specified up in the dataflow are performed together so that the dataset traversal only needs to happen once.
Inside the multithreaded entry-loop, each action is performed for a given entry *only if needed* in order maximize CPU usage and efficiency; this is especially relevant for analysis workflows not bottle-necked by I/O bandwidth.

## Why not DataFrames?

The key distinction between dataflow and DataFrame models is in the layout of the underlying dataset.
DataFrames typically target a specific layout in which each column can be represented as a numerical or array data type, which also enables vectorized operations on columns.
This covers a wide majority of data analysis workflows as evident from the widespread use of such libraries; however, it may not be suitable for instances in which columns contain non-trivial and/or highly-nested data.
Not only are these scenarios are not so uncommon as listed above, trying to shoehorn DataFrame methods to fit their needs are unwieldy at best and could even result in worse performance (single-threaded loops).
Therefore, `queryosity` foregoes the array-wise approach (along with SIMD) and adopts a row-wise one (return to monke...) in favor of manipulating columns of arbitrary data types.

Other variations of the DataFrame model that support more general column data types include (based on author's knowledge and understanding): 

- [C++ DataFrame](https://htmlpreview.github.io/?https://github.com/hosseinmoein/DataFrame/blob/master/docs/HTML/DataFrame.html): custom data types up to a certain register size and memory layout (i.e. no pointers!) using custom memory allocators.
- [Awkward Array](https://awkward-array.org/doc/main/): generalization of the array API to enable key lookups for nested traits while preserving SIMD where possible. 

While these further enhances the universality of the DataFrame methods, they are not panaceas as long as there is *some* data out there too non-trivial to be manipulated in such fashion.