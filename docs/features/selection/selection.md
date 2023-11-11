# Applying selections

## Cuts and weights

A selection can be applied by providing a "path" string and a column that correspond to the decision:
```{ .cpp .annotate }
auto decision = ds.read<bool>("decision");
auto filtered = df.filter("cut_on_decision")(decision);
```

Alternatively, one can apply a weight as:
```{ .cpp .annotate }
auto w = ds.read<float>("weight");
auto filtered_n_weighted = filtered.weight("weight")(w);
```

!!! info 
    Calling a subsequent `filter()/weight()` operation from an existing selection node compound it on top of the chain.
    In the example above, the final cut and weight decisions are:
    ```cpp
    cut = decision && true;
    weight = 1.0 * weight;
    ```

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

## Branching

Selections can branch out from a common one as:
```{ .cpp .annotate }
auto inclusive = df.filter("inclusive")(df.constant(true));
auto filtered_a = inclusive.filter("a")(a);
auto filtered_b = inclusive.filter("b")(b);
```

## (Advanced) Joining

Consider an arbitrary set of selections in a cutflow. Taking the AND/OR of them is commonly required, including scenarios such as:

- AND: Studying overlap between two regions.
- OR: Consolidating two non-orthogonal signal regions into one.

In other libraries, these typically must be done by the error-prone and arduous approach of defining a separate branch of selections, or sometimes even re-structuring of the entire cutflow.
Here, they can be easily done "post-selection" as:

```cpp
auto even_entries = df.filter("even")(entry_number % df.constant(2));
auto odd_entries = df.filter("odd")(!(entry_number % df.constant(2)));
auto third_entries = df.filter("third")(entry_number % df.constant(3));

auto all_entries = df.filter("all")(even_entries || odd_entries);
auto sixth_entries = df.filter("sixth")(even_entries && third_entries);
```

!!! tip
    Treat a selection as a column whose output value is its decision; with that in mind, it is clear as to why the above example should work so simply.