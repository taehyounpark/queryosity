# Applying selections

Call queryosity::dataflow::filter() or queryosity::dataflow::weight() to initiate a selection in the cutflow, and apply subsequent selections from existing nodes to compound them. 

```cpp
auto [w, cat] = ds.read(dataset::column<double>("weight"),
                        dataset::column<std::string>("category"));
auto a = df.define(column::constant<std::string>("a"));
auto b = df.define(column::constant<std::string>("b"));
auto c = df.define(column::constant<std::string>("c"));

// initiate a cutflow
auto weighted = df.weight(w);

// cuts and weights can be compounded in any order.
auto cut =
    weighted.filter(column::expression([](double w) { return (w >= 0;); }))(w);

// applying more than one selection from a node creates a branching point.
auto cut_a = cut.filter(cat == a);
auto cut_b = cut.filter(cat == b);
auto cut_c = cut.filter(cat == c);

// selections can be merged based on their decision values.
auto cut_a_and_b = df.filter(cut_a && cut_b);
auto cut_b_or_c = df.filter(cut_b || cut_c);
```