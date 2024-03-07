:heart: [`Boost.Histogram`](https://www.boost.org/doc/libs/1_84_0/libs/histogram/doc/html/index.html)

## Make a plan


```cpp
auto q = df.get( query::output</*(1)!*/>(/*(2)!*/) );
```

1. `query::definition<Out(Cols...)>` implementation.
2. Constructor arguments for implementation.

```{.cpp .no-copy}

using h1d = qty::hist<float>;
using h2d = qty::hist<float,float>;
namespace axis = qty::hist::axis;

auto q = df.get( query::plan<h1d>(axis::linear(10,0.0,1.0)) );
```

## Fill with columns

A query can be populated ed with input columns as many times as needed...

=== "Fill 1D histogram twice per-entry"
    ```cpp
    auto qh_xy = df.get( 
        query::output<hist<float>>(
            axis::linear(10,0,1.0)
        ),
     ).fill(x).fill(y);
    ```
=== "Fill 2D scatter histogram once per-entry"
    ```cpp
    auto qs_xy = df.get( 
        query::output<hist<float>>(
            axis::linear(10,0,1.0),
            axis::linear(10,0,1.0)
        ),
     ).fill(x,y);
    ```

... As long as the dimensionality of each fill is appropriate:

=== "Valid"

    ```cpp
    auto qs_xy = df.get( 
        query::output<hist<float,float>>(
            axis::linear(10,0,1.0),
            axis::linear(10,0,1.0)
        ),
     ).fill(x,y);
    ```
=== "Not valid"

    ```cpp
    auto qs_xy = df.get( 
        query::output<hist<float,float>>(
            axis::linear(10,0,1.0),
            axis::linear(10,0,1.0)
        ),
     ).fill(x,y,z); // no third dimension (z) exists!
    ```

## Book over selections

The query must be "booked" for execution over the set of entries corresponding to a selection cut.
This also informs the query of the statistical weight of each entry to be taken into account.

```cpp
// 1.
auto q = df.get(/*(1)!*/).fill(/*(2)!*/).book(/*(3)!*/);
```

1. See [Make](#create)
2. See [Fill](#fill)
3. Query is executed over the subset of entries for which the selection cut passes.

A query can be booked at (multiple) selection(s):

=== "Single selection"
    ```cpp
    auto hx_a = df.get( query::output<hist<float>>(axis::linear(10,0,1.0)) ).fill(x).book(sel_a);
    ```
=== "Multiple selections"
    ```cpp
    auto hx = df.get( query::output<hist<float>>(axis::linear(10,0,1.0)) ).fill(x);
    auto [hx_a, hx_b, hx_c] = hx.book(sel_a, sel_b, sel_c);
    ```

Conversely, queries can be booked from a selection node:

=== "Single query"
    ```cpp
    auto hx = df.get( query::output<hist<float>>(axis::linear(10,0,1.0)) ).fill(x);
    auto hx_a = sel_a.book(hx);
    ```
=== "Multiple queries"
    ```cpp
    auto qhx = df.get( query::output<h1d>(axis::linear(10,0,1.0)) ).fill(x);
    auto qsxy = df.get( 
        query::output<h2d>(
            axis::linear(10,0,1.0),
            axis::linear(10,0,1.0)
        ),
     ).fill(x,y);
    auto [hx_a, sxy_a] = a.book(qhx, qsxy);
    ```

## Access results

Requesting result of a lazy query turns it eager and triggers the dataset traversal to collect its entries:
```cpp
auto hist_result = hist.result();  // std::shared_ptr to boost::histogram
```
For syntactical brevity, the lazy query node can be treated as a pointer to its result:
```cpp
hist->at(0);  // equivalent to hist.result()->at(0);
```

!!! info
    The `result()` of any one query triggers the execution of *all* lazy actions.