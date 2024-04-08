# Input datasets

A dataflow needs at least one input dataset with rows to loop over.
Presumably, the dataset also has columns containing some data to analyze for each entry.
Arbitrary dataset formats and column data can be supported by implementing their respective ABCs.

```{admonition} Template
:class: note
```{code} cpp
auto ds = df.load(dataset::input<FORMAT>(ARGUMENTS...));
auto col = ds.read(dataset::column<DTYPE>(NAME));
```

```{seealso}
- [`dataset::reader`](#dataset-reader)
- [`dataset::column`](#dataset-column)
```

## Loading-in a dataset

Call `queryosity::dataflow::load()` with an input dataset and its constructor arguments.

```cpp
using json = qty::json;

std::ifstream data_json("data.json");
auto ds = df.load(dataset::input<json>(data_json));
```

## Reading-out columns

The loaded dataset can then read any valid column, provided its name and data type.
```cpp
auto x = ds.read(dataset::column<double>("x"));
```

There are several shortcuts provided to read a set of columns at once:
::::{tab-set}
:::{tab-item} From a dataset
```cpp
auto [w, cat] = ds.read(dataset::column<double>("weight"),
                        dataset::column<std::string>("category"));
```
:::
:::{tab-item} From the dataflow
```cpp
auto [x, w, cat] =
    df.read(dataset::input<json>(data_json), 
            dataset::column<double>("x"),
            dataset::column<double>("weight"),
            dataset::column<std::string>("category"));
```
```{caution}
An input dataset should be loaded-in *once*, i.e. make sure whatever columns being read-out from the dataflow are *all* the columns of interest from the input dataset.
```
:::
::::

## Working with multiple datasets

A dataflow can load multiple datasets (of different input formats) into one dataflow.

:::{card} Loading JSON and CSV datasets side-by-side.
```{image} ../images/json_csv.png
```
:::

```{code} cpp
std::ifstream data_csv("data.csv");

using csv = qty::csv;
auto y = df.read(dataset::input<csv>(data_csv), dataset::column<double>("y"));

// x from json, y from csv
auto z = x + y;
```

```{admonition} Dataset partition requirements
:class: important
When multiple datasets are loaded into a dataflow, the `queryosity::dataset::source::partition()` implementation of each dataset **MUST** collectively satisfy:
- All non-empty partitions **MUST** have the same total number of entries.
  - If the sub-range boundaries are not aligned with one another, then a common denominator partition with only sub-range boundaries present across all partitions is determined and used in parallelizing the dataflow.
- A dataset can report an empty partition to relinquish the control of the entry loop to the other dataset(s) in the dataflow.
  - Thus, there **MUST** be at least one dataset that reports a non-empty partition.
  - The dataset with an empty partition, as well as its columns, **MUST** remain in a valid state for traversing over any entry numbers as dictated by the other dataset(s).
```