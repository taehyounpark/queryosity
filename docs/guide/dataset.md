# Loading datasets

Call queryosity::dataflow::load() with an input dataset and its constructor arguments.
The loaded dataset can then read out columns, provided their data types and names.

```cpp
std::ifstream data_json("data.json");

// load a dataset
using json = qty::json;
auto ds = df.load(dataset::input<json>(data_json));

// read a column
auto x = ds.read(dataset::column<double>("x"));

// shortcut: read multiple columns from a dataset
auto [w, cat] = ds.read(dataset::column<double>("weight"), dataset::column<std::string>("category"));
```

A dataflow can load multiple datasets, as long as all valid partitions reported by queryosity::dataset::source::partition() have the same number of total entries.
A dataset can report an empty partition, which signals that it relinquishes the control to the other datasets.

```cpp
std::ifstream data_csv("data.csv");

// another shortcut: load dataset & read column(s) at once
using csv = qty::csv;
auto y = df.read(dataset::input<csv>(data_csv), dataset::column<double>("y"));

// x from json, y from csv
auto z = x + y; // (see next section)
```
