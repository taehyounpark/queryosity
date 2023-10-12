# Applying selections

## Cuts and weights

A selection can be applied by providing a "path" string and a column that correspond to the decision:
```{ .cpp .annotate }
auto decision = ds.read<bool>("cut");
auto cut_applied = df.filter("cut")(decision);
```

!!! note "Column names and selection paths"

    The overlap in naming of a dataset column versus a selection path that occurs above is inconsequential. In general, one only has to ensure that the identifiers are unique within each operation type. Even so, the interface never forbids the user from using non-unique identifiers, except for when [bookkeeping multiple selections](../aggregation/result.md).

Alternatively, one can apply a weight as:
```{ .cpp .annotate }
auto w = ds.read<float>("weight");
auto cut_and_weighted = cut_applied.weight("weight")(w);
```

!!! note "Compounding selections"

    Notice that the second `weight` was called from a selection node, not the dataflow object; this compounds those two cuts, which is in this case:
    ```cpp
    cut = decision && true;
    weight = 1.0 * weight;
    ```
    will be applied for each entry.

Also, The selection can optionally receive an expression as an argument:

=== "This is equivalent..."
    ```cpp
    auto two = df.constant(2);
    auto weighted_even_events = cut_and_weighted.filter("even")(entry_index % two);
    ```
=== "To this."
    ```cpp
    auto even_entries = df.filter("even",[](unsigned int index){
      return index % 2;
      })(entry_index);
    ```

## Example cutflow

A non-trivial cutflow such as the one shown in the previous section (and even much more complicated ones) can straightforwardly be expressed as the following:
```{ .cpp .annotate }

auto cut_inclusive = df.filter("inclusive")(inclusive);

auto weighted_w = inclusive.weight("w")(w);
auto channel_a = weighted_w.channel("a")(a);
auto channel_b = weighted_w.channel("b")(b);

auto region_a = channel_a.filter("region")(r);
auto region_b = channel_b.filter("region")(r);

auto region_z = inclusive.filter("x")(x).weight("y")(y).filter("z")(z);
```

## (Advanced) Joining selections

!!! tip "Selections are columns"
    A selection is just a special type of column whose output value is its decision.