Entries in a dataset can be filtered by a `selection`, which is associated with a decision:

- A boolean decision is a `cut` to determine if the entry should be considered or not. (default)
- A floating-point value is a `weight` to assign a statistical significance to the entry.

A selection can be applied by providing its name and a column that correspond to the decision:

```cpp
auto always_true = df.constant(true);
auto all_entries = df.filter("all_entries")(always_true);
auto even_entries = df.filter("even_entries")(entry_index % df.constant(2));
```
In order to apply a weight instead of a cut, it must be provided as a template parameter.
Furthermore, selections can be applied in sequence to compound them.
```cpp
using weight = ana::selection::weight;
auto event_weight = df.read<float>("event_weight");
auto weighted_events = df.filter<weight>("event_weight")(event_weight);

auto cut_geq2jets = weighted_events.filter("event_has_jet",
  [](std::vector<double> const& v) {return v.size() >= 2;})(jets_pt);
```
To save having to write an extra line of column definition, the `filter()` method itself can optionally receive an expression as an argument:
```cpp
auto jets_pt = df.read<std::vector<double>>("jets_pt");
auto cut_geq2jets = df.filter("event_has_jet",
  [](std::vector<double> const& v) {return v.size() >= 2;})(jets_pt);
```

!!! tip "Selections are columns"

    Selections are a special type of columns. 
    This has important and useful consequences, which are shown in [Joining selections](./join.md).