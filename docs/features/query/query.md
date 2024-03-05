## Create

Queries are defined by providing its concrete implementation and constructor arguments:
```cpp
auto hist = df.get( qty::query::output<qty::hist<float>>(qty::axis::linear(100,0,1.0)) );
```

## Fill

A query can be `fill()`ed with input columns of appropriate dimensionality:

=== "Valid"

    ```cpp
    auto scatter_xy = df.get( 
        qty::query::output<qty::hist<float>>(
            qty::axis::linear(100,0,1.0),
            qty::axis::linear(100,0,1.0)
        ),
     ).fill(x,y);
    ```
=== "Not valid"

    ```cpp
    auto scatter_xy = df.get( 
        qty::query::output<qty::hist<float,float>>(
            qty::axis::linear(100,0,1.0),
            qty::axis::linear(100,0,1.0)
        ),
     ).fill(x,y,z); // no third dimension (z) exists!
    ```

An query can be `fill()`ed as many times as needed:

=== "Fill 2D scatter histogram once per-entry"
    ```cpp
    auto scatter_xy = df.get( 
        qty::query::output<qty::hist<float>>(
            qty::axis::linear(100,0,1.0),
            qty::axis::linear(100,0,1.0)
        ),
     ).fill(x,y);
    ```
=== "Fill 1D histogram twice per-entry"
    ```cpp
    auto hist_xy = df.get<Histogram<float>>("x_and_y",100,0,100).fill(x).fill(y);
    ```

## Book

A query can be booked at (multiple) selection(s):

=== "Single selection"
    ```cpp
    auto hx_a = df.get( qty::query::output<qty::hist<float>>(qty::axis::linear(100,0,1.0)) ).fill(x).book(a);
    ```
=== "Multiple selections"
    ```cpp
    auto hx = df.get( qty::query::output<qty::hist<float>>(qty::axis::linear(100,0,1.0)) ).fill(x);
    auto [hx_a, hx_b, hx_c] = hx.book(a, b, c);
    ```

Conversely, multiple queries can be booked from a single selection:

=== "Single query"
    ```cpp
    auto hx = df.get( qty::query::output<qty::hist<float>>(qty::axis::linear(100,0,1.0)) ).fill(x);
    auto hx_a = sel_a.book(hx);
    ```
=== "Multiple querys"
    ```cpp
    auto hx = df.get( qty::query::output<qty::hist<float>>(qty::axis::linear(100,0,1.0)) ).fill(x);
    auto sxy = df.get( 
        qty::query::output<qty::hist<float,float>>(
            qty::axis::linear(100,0,1.0),
            qty::axis::linear(100,0,1.0)
        ),
     ).fill(x,y);
    auto [hx_a, sxy_a] = a.book(hx, sxy);
    ```

## Access results

Accessing result of a lazy query triggers the dataset traversal, i.e. turns it eager:
```cpp
auto hist_result = hist.result();  // std::shared_ptr to boost::histogram
```
For syntactical brevity, the lazy query node can be treated as a pointer to its result:
```cpp
hist->at(0);  // equivalent to hist.result()->at(0);
```

!!! info
    The `result()` of any one query triggers the execution of *all* lazy actions.