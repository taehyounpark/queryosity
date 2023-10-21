# Applying selections

## Cuts and weights

A selection can be applied by providing a "path" string and a column that correspond to the decision:
```{ .cpp .annotate }
auto decision = ds.read<bool>("decision");
auto cut_applied = df.filter("cut_on_decision")(decision);
```

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

Any valid column can enter as an argument; alternatively, an expression can be provided as an optional argument:
=== "This is equivalent..."
    ```cpp
    auto entry_number = ds.read<unsigned int>("entry_number");
    auto even_entries = df.filter("even")(entry_number % df.constant(2));
    ```
=== "To this."
    ```cpp
    auto entry_number = ds.read<unsigned int>("entry_number");
    auto even_entries = df.filter("even",[](unsigned long long number){
      return number % 2;
      })(entry_index);
    ```

## Example cutflow

The example cutflow from the previous section can be expressed as the following:
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