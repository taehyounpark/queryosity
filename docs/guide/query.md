:heart: [`Boost.Histogram`](https://www.boost.org/doc/libs/1_84_0/libs/histogram/doc/html/index.html)

## Make a plan

```cpp
auto q = df.make( query::plan</*(1)!*/>(/*(2)!*/) );
```

1. `query::definition<Out(Cols...)>` implementation.
2. Constructor arguments for implementation.

```{.cpp .no-copy}

using hist_1d = qty::hist<double>;
using hist_2d = qty::hist<double,double>;
using lin_ax = qty::hist::axis::linear;

auto q = df.make( query::plan<hist_1d>(lin_ax(10,0.0,1.0)) );
```

## Fill with columns

```cpp
auto q = df.make(/*(1)!*/).fill(/*(2)!*/);
```

1. See [Make a plan](#make-a-plan).
2. Input column(s).

A query can be populated ed with input columns as many times per-entry as desired...

=== "Fill 1D histogram twice per-entry"
    ```cpp
    auto q_h1x = df.make( 
        query::plan<hist_1d>(
            lin_ax(10,0,1.0)
            ),
        ).fill(x).fill(y);
    ```
=== "Fill 2D scatter histogram once per-entry"
    ```cpp
    auto q_h2xy = df.make( 
        query::plan<hist_1d>(
            lin_ax(10,0,1.0),
            lin_ax(10,0,1.0)
            ),
        ).fill(x,y);
    ```

... As long as the dimensionality of each fill is appropriate:

=== "Valid"

    ```cpp
    auto q_h2xy = df.make( 
        query::plan<hist_2d>(
            lin_ax(10,0,1.0),
            lin_ax(10,0,1.0)
            ),
        ).fill(x,y);
    ```
=== "Not valid"

    ```cpp
    auto q_h2xy = df.make( 
        query::plan<hist_2d>(
            lin_ax(10,0,1.0),
            lin_ax(10,0,1.0)
        ),
     ).fill(x,y,z);  // compilation error: no third dimension (z) exists.
    ```

## Book over selections

```cpp
auto q = df.make(/*(1)!*/).fill(/*(2)!*/).book(/*(3)!*/);
```

1. See [Make a plan](#create)
2. See [Fill with columns](#fill)
3. Associated selection.

=== "Book a query *at* a selection"
    ```cpp
    auto q = df.make( query::plan<hist_1d>(lin_ax(10,0,1.0)) ).fill(x);
    auto h1x_a = q.book(sel_a);
    ```
=== "Book a query *from* a selection"
    ```cpp
    auto q = df.make( query::plan<hist_1d>(lin_ax(10,0,1.0)) ).fill(x);
    auto h1x_a = sel_a.book(q);
    ```
<!--  -->

=== "Single query at multiple selections"
    ```cpp
    auto q = df.make( query::plan<hist_1d>(lin_ax(10,0,1.0)) ).fill(x);
    auto [h1x_a, h1x_b, h1x_c] = q.book(sel_a, sel_b, sel_c);
    ```
=== "Multiple queries from single selection"
    ```cpp
    auto q_h1x = df.make( query::plan<hist_1d>(lin_ax(10,0,1.0)) ).fill(x);
    auto q_h2xy = df.make( 
        query::plan<hist_2d>(
            lin_ax(10,0,1.0),
            lin_ax(10,0,1.0)
            ),
        ).fill(x,y);
    auto [h1x_a, h2xy_a] = sel_a.book(q_1, q_2);
    ```

## Access results

```cpp
auto q_result = df.make(/*(1)!*/).fill(/*(2)!*/).book(/*(3)!*/).result();
```

1. See [Make a plan](#make-a-plan).
2. See [Fill with columns](#fill-with-columns).
3. See [Book over selections](#book-over-selections).

```{.cpp .no-copy}
h1x_a.result();  // slow: dataset must be processed.
h2x_b.result();  // fast: dataset already processed.
```

!!! tip

    If a query outputs a pointer, the query itself can be treated as the pointer:

    ```{.cpp .no-copy}
    h1x_a.result();  // std::shared_ptr to boost::histogram
    h1x_a->at(0);    // same as hist.result()->at(0);
    ```