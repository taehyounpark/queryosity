Entries in a dataset can be "selected", which is associated with a decision:

- A boolean decision is a `cut` to determine if the entry should be considered or not. (default)
- A floating-point value is a `weight` to assign a statistical significance to the entry.

A selection can be applied by providing its name and a column that correspond to the decision:
```cpp
auto accept_all = df.constant(true);
auto all_entries = df.filter("all")(accept_all);
```
Alternatively, one can apply a weight:
```{ .cpp .annotate }
auto w = ds.read<float>("weight");
auto weighted_events = df.weight("weight")(w);  // (1)
```

1.    The overlap in naming of a dataset column versus selection is inconsequential. In general, any operation can possess a non-unique name. See [next section](./channel.md) and [aggregations](../aggregation/result.md) for where names of selections become relevant.

Furthermore, selections can be applied in sequence to compound them.
```cpp
auto two = df.constant(2);
auto weighted_even_events = weighted_events.filter("even")(entry_index % two);
```
The selection can optionally receive an expression as an argument:
```cpp
auto even_entries = df.filter("even",[](unsigned int index){
  return index % 2;
  })(entry_index);
```
!!! tip "Selections are columns"
    A selection is just a special type of column whose output value is its decision.