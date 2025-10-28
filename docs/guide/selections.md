{#applying-selections}
::::{tab-set}

:::{tab-item} Existing column
:::{card} Template
```cpp
auto cut = df.filter(COL);
auto wgt = df.weight(COL);
```
:::

:::{tab-item} Constant
:::{card} Template
```cpp
auto cut = df.filter(dataset::constant(VAL));
auto wgt = df.weight(dataset::constant(VAL));
```
:::

:::{tab-item} Expression
:::{card} Template
```cpp
auto cut = df.filter(column::expression(FUNC))(COLS...);
auto wgt = df.weight(column::expression(FUNC))(COLS...);
```
:::

:::{tab-item} Definition
:::{card} Template
```cpp
auto cut = df.filter(column::definition<DEF>(ARGS...))(COLS...);
auto wgt = df.weight(column::definition<DEF>(ARGS...))(COLS...);
```
:::

::::


# Initiating a cutflow
Call `dataflow::filter()` or `dataflow::weight()` to initiate a selection in the cutflow.
```cpp
auto all = df.filter(column::constant(true));
```


# Compounding selections

Selections can be compounded onto existing ones regardless of their cut/weight specification:
a cut simply passes through the weight decision of its previous selection (if one exists), and vice versa.

```cpp
auto w = ds.read(dataset::column<double>("weight"));

auto sel = all.weight(w).filter(
    column::expression([](column::observable<double> w) { return (w.value() >= 0;); }))(w);
// cut    = (true) && (true) && (w>=0);
// weight = (1.0)   *    (w)  *  (1.0);
```

# Branching selections

Applying multiple selections from a common node creates a branching in the cutflow.

```cpp
auto cat = ds.read(dataset::column<std::string>("category"));
auto a = df.define(column::constant<std::string>("a"));
auto b = df.define(column::constant<std::string>("b"));
auto c = df.define(column::constant<std::string>("c"));

auto sel_a = sel.filter(cat == a);
auto sel_b = sel.filter(cat == b);
auto sel_c = sel.filter(cat == c);
```

# Joining selections

Any set of selections can be merged back together by `&&`/`||`/`*`-ing them.

```cpp
// why weight(w)? see below
auto sel_a_and_b = df.filter(sel_a && sel_b).weight(w);
auto sel_b_or_c  = df.weight(w).filter(sel_b || sel_c);
```

:::{important}
The mechanism for joining selections is provided through [Basic operations](#computing-columns-operators) between columns.
Therefore, a joined cut/weight constitutes the first selection in a new cutflow, while its weight/cut decision, is discarded.
These can (and should) be re-applied at any point in the new cutflow.
:::

# Yield at a selection

```cpp
// single selection
auto yield_tot = df.get(selection::yield(all));
unsigned long long yield_tot_entries = yield_tot.entries; // number of entries
double yield_tot_value = yield_tot.value; // sum(weights)
double yield_tot_error = yield_tot.error; // sqrt(sum(weights squared))

// multiple selections 
auto [yield_a, yield_b, yield_c] =
    df.get(selection::yield(sel_a, sel_b, sel_c));
```