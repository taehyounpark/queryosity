{#performing-queries}
# Performing queries

There are a total of three steps in fully specifying a query:

1. Definition (concrete type and constructor arguments) of the query itself.
2. Input column(s) with which it is filled with.
3. Associated selection(s) at which it is performed.

```{admonition} Template
:class: note
```{code} cpp
auto q = df.get(query::output<DEFINITION>(ARGUMENTS...))
             .fill(COLUMNS...)
             .at(SELECTIONS...);
```
:::

## Defining a query

Call `queryosity::dataflow::get()` specifying the definition and constructor arguments of the query.

```{code} cpp
using h1d = qty::hist::hist<double>;
using linax = qty::hist::axis::regular;

auto q1 = df.get(query::output<h1d>(linax(100, 0.0, 1.0)));
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
auto q1xy = df.get(query::output<h1d>(linax(10, 0.0, 1.0)))(x)(y);
```
:::
:::{tab-item} 2D histogram filled once per-entry
```{code} cpp
using h2d = qty::hist::hist<double, double>;
auto q2xy =
    df.get(query::output<h2d>(linax(10, 0.0, 1.0), linax(10, 0.0, 1.0)))(x, y);
```
:::
::::

## Booking a query

Booking a query at a particular selection fully instantiates the lazy query:
```{code} cpp
auto all = df.filter(column::constant(true));
auto q1x_all = q1x.at(all);
```

Alternatively, multiple queries can be called in one line in two ways:
::::{tab-set}
:::{tab-item} Book a query at multiple selections
```{code} cpp
auto [q1x_a, q1x_b] = q1x.at(cut_a, cut_b);
```
:::
:::{tab-item} Book multiple queries at a selection
```{code} cpp
auto [q1x_c, q2xy_c] = c.book(q1x, q2xy);
```
:::
::::

## Accessing results

Access the result of a query turn *all* actions in the dataflow---its associated selections and columns, as well as those of all other queries---eager.

```{code} cpp
auto h1x_a = q1x_a.result(); // takes a while
auto h2xy_c = q2xy_c.result(); // instantaneous
```

