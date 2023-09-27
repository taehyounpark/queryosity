Entries in a dataset can be filtered by a `selection`, which is associated with a decision:

- A boolean decision is a `cut` to determine if the entry should be considered or not. (default)
- A floating-point value is a `weight` to assign a statistical significance to the entry.

A selection can be applied by providing its name and a column that correspond to the decision:
```cpp
auto always_true = df.constant(true);
auto all_entries = df.filter("all")(always_true);
```
In order to apply a weight instead of a cut, it must be provided as a template parameter:
```cpp
using weight = ana::selection::weight;
auto w = df.read<float>("weight");
auto weighted_events = df.filter<weight>("weight")(w);
```
Furthermore, selections can be applied in sequence to compound them.
```cpp
auto two = df.constant(2);
auto weighted_even_events = weighted_events.filter("even")(entry_index % two);
```
For method can optionally receive an expression as an argument:
```cpp
auto even_entries = df.filter("even",[](unsigned int index){
  return index % 2;
  })(entry_index);
```
!!! tip "Selections are columns"
    A selection is just a special type of column whose output value is its decision.