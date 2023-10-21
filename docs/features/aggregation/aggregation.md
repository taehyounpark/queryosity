

## Book, fill, at

!!! warning "Make sure to get the returned booker"

    Reminder: each (chained) method returns a new node with the lazy action booked.
    In other words, make sure to obtain and use the returned aggregation for the columns to be actually filled!
    The following will be a mistake:
    ```cpp
    auto hist = df.book<hist::hist<float>>("x",50,0,400);
    hist.fill(x);  // mistake!
    ```

The dimensionality must match that of the implementation:

=== "Valid"

    ```cpp
    auto hist_xy = df.book<hist::hist<float,float>>("xy",100,0,100,100,0,100).fill(x,y);
    ```
=== "Not valid"

    ```cpp
    auto hist_xy = df.book<hist::hist<float,float>>("xy",100,0,100,100,0,100).fill(x,y,z);
    ```
An aggregation can be `fill()`ed as many times as needed:

```cpp title="Filling a histogram twice per-entry"
auto hist_x_y = df.book<hist::hist<float>>("x_y",100,0,100).fill(x).fill(y);
```
!!! tip "Actions on aggregations are "modular""
    An upside of the above caution is the modularity of actions that can be performed on aggregations.
    For example, a common binning of histograms for different variables can be re-cycled:
    ```cpp
    auto bins = df.book<hist::hist<float>>("hist",100,0,100);
    auto hist_x = bins.fill(x);
    auto hist_y = bins.fill(y);
    ```
Similar in spirit to filling columns, an aggregation can be booked at multiple selections:

=== "One selection"
    ```cpp
    hist = df.book<hist::hist<float>>(hist::axis::regular(100,0,100))\
             .fill(x)\
             .at(c);
    ```
=== "Multiple selections"
    ```cpp
    hists = df.book<hist::hist<float>>(hist::axis::regular(100,0,100))\
              .fill(x)\
              .at(a, b, c);
    ```

## Booking from selections

## Accessing the result(s)

The result of any single aggregation can be triggered by:
```cpp
auto hist = df.book<hist<float>>(100,0,100).fill(column).at(selection);
auto hist_result = hist.result();  // std::shared_ptr of boost::histogram
```

For outputs that are not basic data types, they can also be de-referenced or pointed to:
```cpp
hist->at(0);  // first bin value
```

!!! note
    Calling the result of any one aggregation triggers the dataset processing of *all*.