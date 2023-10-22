## Create

!!! warning "Make sure to get the returned booker"

    Reminder: each (chained) method returns a new node with the lazy action booked.
    In other words, make sure to obtain and use the returned aggregation for the columns to be actually filled!
    The following will be a mistake:
    ```cpp
    auto hist = df.agg<hist::hist<float>>("x",50,0,400);
    hist.fill(x);  // mistake!
    ```

## Fill

The dimensionality must match that of the implementation:

=== "Valid"

    ```cpp
    auto x_vs_y = df.agg<hist::hist<float,float>>("xy",100,0,100,100,0,100).fill(x,y);
    ```
=== "Not valid"

    ```cpp
    auto x_vs_y = df.agg<hist::hist<float,float>>("xy",100,0,100,100,0,100).fill(x,y,z);
    ```

An aggregation can be `fill()`ed as many times as needed:

```cpp title="Filling a histogram twice per-entry"
auto x_and_y = df.agg<hist::hist<float>>("x_and_y",100,0,100).fill(x).fill(y);
```
!!! info "Breaking up aggregation calls"
    Method chaining can be broken up to modularize calls on aggregations.
    For example, a common axis binning can be recycled for multiple histograms of different variables:
    ```cpp
    auto hbins = df.agg<hist::hist<float>>("hist",100,0,100);
    auto hx = hbins.fill(x);
    auto hy = hbins.fill(y);
    ```

## Book

An aggregation can be booked at (multiple) selection(s):

=== "One selection"
    ```cpp
    hx_c = df.agg<hist::hist<float>>(hist::axis::regular(100,0,100))\
             .fill(x)\
             .book(c);
    ```
=== "Multiple selections"
    ```cpp
    hxs = df.agg<hist::hist<float>>(hist::axis::regular(100,0,100))\
              .fill(x)\
              .book(a, b, c);
    ```

### From a selection

Existing aggregations can also be booked from a selection, which makes it easier (syntactically) to keep track of multiple aggregations booked at a single selection.
```cpp
auto hx = df.agg<hist::hist<float>>("x",axis::regular(10,0,10));
auto hxy = df.agg<hist<float,float>>("xy",axis::regular(10,0,10),axis::regular(10,0,10));

auto [hx_a, hxy_a] = sel_a.book(hx, hxy);
```

## Access result(s)

The result of any single aggregation can be triggered by:
```cpp
auto hist = df.agg<hist<float>>(100,0,100).fill(column).book(selection);
auto hist_result = hist.result();  // std::shared_ptr of boost::histogram
```

For outputs that are not basic data types, they can also be de-referenced or pointed to:
```cpp
hist->at(0);  // boost::histogram::at(): get bin value
```

!!! note
    Calling the result of any one aggregation triggers the dataset processing of *all*.