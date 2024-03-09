@mainpage
@tableofcontents

`queryosity` is row-wise data analysis library written in & for C++.

<!-- --------------------------------------------------------------------------------------------------------------- -->

@section mainpage-features Features

- **Dead simple.** The easy-to-learn, self-consistent API has a grand total of *five* endpoints to perform even the most complex operations.
- **Arbitrary actions.** Work with datasets/columns/queries of *any* data structure the way *you* want to.
- **Lazy but efficient.** An action is performed for an entry only if needed. All actions are performed in one dataset traversal. Dataset traversal is multithreaded.
- **Systematic variations.** Perform *automatic* sensitivity analysis by propagating systematic variations through actions.

<!-- --------------------------------------------------------------------------------------------------------------- -->

@section mainpage-installation Installation

The following compilers with C++17 support are part of the CI.

| OS | Compiler | Versions |
| :--- | :--- | :--- |
| macOS 12 | Clang | 13.1, 13.2.1, 13.3.1, 13.4.1, 14.0, 14.0.1, 14.1 |
| Ubuntu 22.04 LTS | GCC | 9.4, 10.5, 11.4, 12.3 |

@subsection mainpage-installation-header Single-header

~~~{.cpp}
#include "queryosity.h"
~~~

@subsection mainpage-installation-header CMake

~~~{.sh}
git clone https://github.com/taehyounpark/queryosity.git
~~~

### Standalone

~~~{.sh}
cd queryosity/ && mkdir build/ && cd build/
cmake ../
cmake --build .
cmake --install .
~~~

~~~{.cmake}
find_package(queryosity 0.1.0 REQUIRED)
...
add_library(YourProject ...)
...
target_link_libraries(YourProject INTERFACE queryosity::queryosity)
~~~

~~~{.cpp}
#include "queryosity/queryosity.h"
~~~

### Integrated

~~~{.cmake}
add_subdirectory(queryosity)
...
add_library(YourProject ...)
...
target_link_libraries(YourProject INTERFACE queryosity::queryosity)
~~~
~~~{.cpp}
#include "queryosity/queryosity.h"
~~~

@section mainpage-design-goals Design goals

`queryosity` provides a "dataflow", or row-wise, model of structured data analysis.
Users specify operations as if they are inside for-loop over the entries of the dataset.
The key is to *specify* without executing the operations on a dataset until they are needed, i.e. create *lazy* actions.
Each lazy actions in a dataflow are only if needed in order maximize CPU usage and efficiency (see [Lazy actions](../concepts/lazy.md)), which is especially relevant for analysis workflows not limited by I/O bandwidth.

This library has been purposefully developed for high-energy physics experiments; hopefully, it can be similarly useful for studying other complex phenomena.

### Why not DataFrames?

Two key distinctions separate the dataflow against the plethora of DataFrame libraries.

#### Conceptual

DataFrames can do both array-wise and row-wise operations, but the former mode is where it shines, whereas the latter is more of a fallback.
The result is a complicated API where careless mixing-and-matching of the two approaches will cost the user their sanity.

Other the other hand, putting row-wise reasoning at the forefront is the intuitive way of thinking about tabular datasets for humans (return to monke...).
This greatly simplifies the dataflow API, which is also distinguished by its syntax faithfully representing the actual computation graph being performed inside each entry.
In other words, Alice's analysis code can *actually* be readable to Bob, and vice versa!

#### Technical

DataFrames are optimized for a specific dataset structure in which the values in each column can be organized into a contiguous array.
Operations on these arrays of primitive data types can offer algorithmic (e.g. linear vs. binary search) as well as hardware (e.g. vectorized operations) speedups.

(i.e. If a dataset fits into a DataFrame, it will almost always be better for the job).

While this covers a wide range of data analysis workflows as evident from the widespread use of these libraries, it is not suitable for instances in which columns contain non-trivial and/or highly-nested data.
This is not so uncommon as listed above.
"Extensions" to DataFrames to support more complex column data types do exist, but they are not true panaceas as long as there is *some* data out that cannot be vectorized.

No such restrictions need exist with dataflow, as promised to be one of its main features.
It also does the best it can in terms of performance through lazy actions and multithreading.
