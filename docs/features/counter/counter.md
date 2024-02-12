## Create

Aggregations are defined analogous as for custom column definitions, i.e. by providing its concrete type and constructor arguments:
```cpp
auto hist = df.agg<Histogram<float>>(LinearAxis(100,0,100));
```

## Fill

An counter must be `fill()`ed with input columns of matching dimensionality:
=== "Valid"

    ```cpp
    auto scatter_xy = df.agg<Histogram<float,float>>("xy",100,0,100,100,0,100).fill(x,y);
    ```
=== "Not valid"

    ```cpp
    auto scatter_xy = df.agg<Histogram<float,float>>("xy",100,0,100,100,0,100).fill(x,y,z);
    ```

An counter can be `fill()`ed as many times as needed:

```cpp title="Filling a histogram twice per-entry"
auto hist_xy = df.agg<Histogram<float>>("x_and_y",100,0,100).fill(x).fill(y);
```
<!-- !!! warning "Make sure to get the returned booker"

    Reminder: each (chained) method returns a new node with the lazy action booked.
    In other words, make sure to obtain and use the returned counter for the columns to be actually filled!
    The following will be a mistake:
    ```cpp
    auto hist = df.agg<Histogram<float>>(LinearAxis(100,0,100));
    hist.fill(x);  // mistake!
    ```

!!! info "Breaking up counter calls"
    On the flip side of the above warning method chaining can be used to break up counter definitions.
    For example, a common axis binning can be recycled for multiple histograms of different variables:
    ```cpp
    auto hbins = df.agg<Histogram<float>>(LinearAxis(100,0,100));
    auto hx = hbins.fill(x);
    auto hy = hbins.fill(y);
    ``` -->

## Book

An counter can be booked at (multiple) selection(s):

=== "One selection"
    ```cpp
    hx = df.agg<Histogram<float>>(LinearAxis(100,0,100)).fill(x);

    hx_a = hx.book(sel_a);
    ```
=== "Multiple selections"
    ```cpp
    hx = df.agg<Histogram<float>>(LinearAxis(100,0,100)).fill(x);
    hx_abc = hx.book(a, b, c);
    hx_a = hx_abc["a"];
    ```

When multiple counters are booked from a selection, they must be individually unpacked since counters can be of different types:

=== "One counter"
    ```cpp
    auto hx = df.agg<Histogram<float>>("x",LinearAxis(10,0,10));

    auto hx_a = sel_a.book(hx);
    ```
=== "Multiple counters"
    ```cpp
    auto hx = df.agg<Histogram<float>>("x",LinearAxis(10,0,10));
    auto hxy = df.agg<Histogram<float,float>>("xy",LinearAxis(10,0,10),LinearAxis(10,0,10));
    auto [hx_a, hxy_a] = sel_a.book(hx, hxy);
    ```

## Access result(s)

The result of any single counter can be obtained as:
```cpp
auto hist = df.agg<Histogram<float>>(100,0,100).fill(column).book(selection);
auto hist_result = hist.result();  // std::shared_ptr to boost::histogram object
```
Which triggers the execution of all lazy actions in the dataflow object over the dataset.

The counter behaves as a pointer to its result, which is automatically triggered:
```cpp
hist->at(0);  // equivalent to hist.result()->at(0);
```

!!! info
    Calling the result of any one counter triggers the dataset processing of *all* booked up to that point.