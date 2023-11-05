## Create

Aggregations are defined analogous as for custom column definitions, i.e. by providing its concrete type+constructor arguments:
```cpp
auto hist = df.agg<Histogram<float>>(LinearAxis(100,0,100));
```


## Fill

An aggregation must be `fill()`ed with input columns of matching dimensionality:
=== "Valid"

    ```cpp
    auto scatter_xy = df.agg<Histogram<float,float>>("xy",100,0,100,100,0,100).fill(x,y);
    ```
=== "Not valid"

    ```cpp
    auto scatter_xy = df.agg<Histogram<float,float>>("xy",100,0,100,100,0,100).fill(x,y,z);
    ```

An aggregation can be `fill()`ed as many times as needed:

```cpp title="Filling a histogram twice per-entry"
auto hist_xy = df.agg<Histogram<float>>("x_and_y",100,0,100).fill(x).fill(y);
```

!!! warning "Make sure to get the returned booker"

    Reminder: each (chained) method returns a new node with the lazy action booked.
    In other words, make sure to obtain and use the returned aggregation for the columns to be actually filled!
    The following will be a mistake:
    ```cpp
    auto hist = df.agg<Histogram<float>>(LinearAxis(100,0,100));
    hist.fill(x);  // mistake!
    ```

!!! info "Breaking up aggregation calls"
    On the flip side of the above warning method chaining can be used to break up aggregation definitions.
    For example, a common axis binning can be recycled for multiple histograms of different variables:
    ```cpp
    auto hbins = df.agg<Histogram<float>>(LinearAxis(100,0,100));
    auto hx = hbins.fill(x);
    auto hy = hbins.fill(y);
    ```


## Book

An aggregation can be booked at (multiple) selection(s):

=== "One selection"
    ```cpp
    hx_c = df.agg<Histogram<float>>(LinearAxis(100,0,100))\
             .fill(x)\
             .book(c);
    ```
=== "Multiple selections"
    ```cpp
    hxs = df.agg<Histogram<float>>(LinearAxis(100,0,100))\
              .fill(x)\
              .book(a, b, c);
    ```

### From a selection

When multiple aggregations are booked from a selection, they must be individually unpacked since aggregations can be of different types:

```cpp
auto hx = df.agg<Histogram<float>>("x",axis::regular(10,0,10));
auto hxy = df.agg<Histogram<float,float>>("xy",axis::regular(10,0,10),axis::regular(10,0,10));

auto [hx_a, hxy_a] = sel_a.book(hx, hxy);
```

## Access result(s)

The result of any single aggregation can be triggered by:
```cpp
auto hist = df.agg<Histogram<float>>(100,0,100).fill(column).book(selection);
auto hist_result = hist.result();
```

De-referencing the aggregation automatically triggers its result:
```cpp
hist->at(0);  // first bin content
```

!!! note
    Calling the result of any one aggregation triggers the dataset processing of *all*.