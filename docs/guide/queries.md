# Performing queries

Call queryosity::dataflow::get() specifying the exact definition and constructor arguments of the query.
Subsequently, it can be filled with input columns and booked at a selection to instantiate the query:

```cpp
using h1d = qty::hist::hist<double>;
using h2d = qty::hist::hist<double,double>;
using linax = qty::hist::axis::regular;

auto q = df.get(query::output<h1d>(linax(100,0.0,1.0))).fill(x).at(cut);
```

A query can be filled multiple times, as long as the dimensionality of each call is appropriate:

```cpp
// fill 1d histogram with x & y for each entry
auto q_1xy = df.get(query::output<h1d>(linax(10, 0.0, 1.0)))(x)(y);

// fill 2d histogram with (x, y) for each entry
auto q_2xy =
    df.get(query::output<h2d>(linax(10, 0.0, 1.0), linax(10, 0.0, 1.0)))(x, y);
```

Multiple queries can be instantiated at once by:

1. Performing a specific query at multiple selections.
2. Performing multiple (different) queries from a selection.

```cpp
// 1.
auto [q_1xy_a, q_1xy_b] = q_1xy.at(cut_a, cut_b);

// 2.
auto [q_1xy_c, q_2xy_c] = c.book(q_1xy, q_2xy);
```

Access the result of a query to turn all actions eager.

```cpp
auto h1x_a = q_1xy_a.result(); // takes a while
auto h2xy_c = q_2xy_c.result(); // instantaneous
```