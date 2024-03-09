# Selections

## Applying cuts and weights

Applying a selection from the `dataflow` object applies it over the inclusive dataset.

```cpp
// 1.
auto cut = df.filter(/*(1)!*/);
```

1. Existing column whose is used as its decision.

```cpp
// 2.
auto wgt = df.weight( column::expression(/*(1)!*/), /*(2)!*/);
```

1. See [Expression](./column.md#expression).
1. See [Expression](./column.md#expression).

<!--  -->

1. Creating a selection out of an existing column.
2. A syntactic shortcut use an expression.

## Compounding selections

Call a subsequent selection from an existing selection node compounds them.
Cuts and weights can be inter-compounded:
```{.cpp .no-copy}
auto [c, w] = ds.read(dataset::columns<bool,double>("c","w"));

auto cut_n_wgt = df.filter(c).weight(w);
// cut_n_wgt.passed_cut() = c.value() && true;
// cut_n_wgt.get_weight() = 1.0 * w.value();
```

## Branching selections

Multiple selections can be compounded from one common node:
```{.cpp .no-copy}
auto [a, b] = ds.read(dataset::columns<bool,bool>("a","b"));

auto cut_a = df.filter(a);  // a
auto cut_ab = a.filter(b);  // a && b
auto cut_ac = a.filter(c);  // a && c
```

## Merging selections

Consider two selections within a cutflow. Taking the AND/OR of them is commonly required, including scenarios such as:

- AND: Quantifying overlap between two selections.
- OR: Consolidating two not-mutually-exclusive selections into one.

```{.cpp .no-copy}
auto index = df.index();

auto one = df.define( column::constant<unsigned int>(1) );
auto two = df.define( column::constant<unsigned int>(2) );
auto three = df.define( column::constant<unsigned int>(3) );

auto even = df.filter(index % two == one);
auto odd = df.filter(!even);
auto third = df.filter(index % three == two);

auto all = df.filter(even || odd);
auto none = df.filter(even && odd);
auto sixth = df.filter(even && third);
```

!!! tip

    - Treat a selection as a column whose output value is its decision; then it becomes clear as to why the above examples work so seamlessly.
    - Conversely, when a particular selection (cut or weight) is used as a column, *the other decision is ignored* (i.e. weight or cut, respectively).
        - If and where needed, the other decision must be re-applied from the newly-applied selection node.