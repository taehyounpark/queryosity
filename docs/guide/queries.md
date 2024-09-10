{#performing-queries}
# Performing queries

There are a total of three steps in fully specifying a query:

1. Definition (concrete type and constructor arguments) of the query itself.
2. Input column(s) with which it is filled with.
3. Associated selection(s) at which it is performed.

:::{card} Template
```{code} cpp
auto q = df.get(query::result<DEF>(ARGS...))
             .fill(COLS...)
             .at(SELS...);
```
:::

## Defining a query

Call `dataflow::get()` specifying the definition and constructor arguments of the query.

```{code} cpp
using h1d = qty::hist::hist<double>;
using linax = qty::hist::axis::regular;

auto q1 = df.get(query::result<h1d>(linax(100, 0.0, 1.0)));
```

## Filling a query

A defined query can be `fill()`ed with input columns:
```{code} cpp
auto q1x = q.fill(x);
```

A query can be filled multiple times, as long as the dimensionality of each fill is valid:
::::{tab-set}
:::{tab-item} 1D histogram filled twice per-entry
```{code} cpp
auto q1xy = df.get(query::result<h1d>(linax(10, 0.0, 1.0)))(x)(y);
```
:::
:::{tab-item} 2D histogram filled once per-entry
```{code} cpp
using h2d = qty::hist::hist<double, double>;
auto q2xy =
    df.get(query::result<h2d>(linax(10, 0.0, 1.0), linax(10, 0.0, 1.0)))(x, y);
```
:::
::::

## Booking a query

Booking a query at a particular selection fully instantiates the lazy query:
```{code} cpp
auto all = df.filter(column::constant(true));
auto q1x_all = q1x.at(all);
```

A given query can be booked at multiple selections:
```{code} cpp
auto [q1x_a, q1x_b] = q1x.at(cut_a, cut_b);
```

Or, multiple different queries can be booked from a selection:
```{code} cpp
auto [q1x_c, q2xy_c] = cut.book(q1x, q2xy);
```

## Accessing results

Access the result of any query to trigger the dataset traversal for all.

```{code} cpp
auto h1x_a = q1x_a.result(); // takes a while
auto h2xy_c = q2xy_c.result(); // instantaneous
```

