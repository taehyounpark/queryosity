# Applying selections

## Cuts and weights

Applying a selection from the `dataflow` object applies it over the inclusive dataset.

```cpp
// 1.
auto filtered = df.filter(/*(1)!*/);
```

1. Existing column whose is used as its decision.

```cpp
// 2.
auto weighted = df.weight( column::expression(/*(1)!*/), /*(1)!*/);
```

1. See [Expression](./column.md#expression).
2. See [Expression](./column.md#expression).

<!--  -->

1. The simplest way to apply a selection is using an existing column.
2. A syntactic shortcut to first evaluate a column expression is available.

## Compounding selections

Calling a subsequent `filter()` and `weight()` action from an existing selection node compound it on top of the chain.
Either can be inter-compounded, as in:
```{.cpp .no-copy}
auto [c, w] = ds.read(
    dataset::column<bool>("c"),
    dataset::column<double>("w")
)

auto cut_n_weighted = df.filter(c).weight(w);
// cut_n_weighted.passed_cut() = c.value() && true;
// cut_n_weighted.get_weight() = 1.0 * w.value();
```

## Branching selections

Multiple selections can be compounded from one common node, i.e. selections can branch out:
```{.cpp .no-copy}
auto [a, b] = ds.read(
    dataset::column<bool>("a"),
    dataset::column<bool>("b"),
);

auto cut_a = df.filter(a);
auto cut_ab = a.filter(b);
auto cut_ac = a.filter(c);
```


## Merging selections

Consider two selections within a cutflow. Taking the AND/OR of them is commonly required, including scenarios such as:

- AND: Quantifying overlap between two selections.
- OR: Consolidating two not-mutually-exclusive selections into one.

```cpp
auto two = df.define( column::constant<unsigned int>(2) );
auto three = df.define( column::constant<unsigned int>(3) );

auto even_entries = df.filter(entry_number % two);
auto odd_entries = df.filter(!even_entries);
auto third_entries = df.filter(entry_number % three);

auto all_entries = df.filter(even_entries || odd_entries);
auto sixth_entries = df.filter(even_entries && third_entries);
```

!!! tip

    - Treat a selection as a column whose output value is its decision; then it becomes clear as to why the above examples work so seamlessly.
    - Conversely, when a particular selection (cut or weight) is used as a column, *the other decision is ignored* (i.e. weight or cut, respectively).
        - If and where needed, the other decision must be re-applied from the newly-applied selection node.