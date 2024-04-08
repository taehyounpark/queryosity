{#applying-selections}
# Applying selections

## Initiate a cutflow

Call queryosity::dataflow::filter() or queryosity::dataflow::weight() to initiate a selection in the cutflow

```cpp
auto [w, cat] = ds.read(dataset::column<double>("weight"),
                        dataset::column<std::string>("category"));
auto a = df.define(column::constant<std::string>("a"));
auto b = df.define(column::constant<std::string>("b"));
auto c = df.define(column::constant<std::string>("c"));

// initiate a cutflow
auto weighted = df.weight(w);
```

## Compounding selections

Subsequently-compounded selections from existing ones can be applied by chained `filter()`/`weight()` calls. 
```cpp
// cuts and weights can be compounded in any order.
auto cut =
    weighted.filter(column::expression([](double w) { return (w >= 0;); }))(w);
```

## Branching selections

```cpp
// applying more than one selection from a node creates a branching point.
auto cut_a = cut.filter(cat == a);
auto cut_b = cut.filter(cat == b);
auto cut_c = cut.filter(cat == c);
```

## Merging selections

```cpp
// selections can be merged based on their decision values.
auto cut_a_and_b = df.filter(cut_a && cut_b);
auto cut_b_or_c = df.filter(cut_b || cut_c);
```
## Yield at a selection

```cpp
// single selection
auto all = df.filter(column::constant(true));
auto yield_tot = def.get(selection::yield(all));
unsigned long long yield_tot_entries =
    yield_tot.entries;                    // number of entries passed
double yield_tot_value = yield_tot.value; // sum(weights)
double yield_tot_error = yield_tot.error; // sqrt(sum(weights squared))

// multiple selections 
// (sel_a/b/c: (varied) lazy selections)
auto [yield_a, yield_b, yield_c] =
    df.get(selection::yield(sel_a, sel_b, sel_c));
```