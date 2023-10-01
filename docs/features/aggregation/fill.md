
An aggregation can be filled with columns any number of times, as long as the dimensionality matches the implementation for each call.

=== "Valid"

    ```cpp
    auto hist_xy = df.book<Hist<2,float>>("xy",100,0,100,100,0,100).fill(x,y,z);
    ```
=== "Not valid"

    ```cpp
    auto hist_xy = df.book<Hist<2,float>>("xy",100,0,100,100,0,100).fill(x);
    ```

```cpp title="Filling a histogram twice per-entry"
auto hist_x_y = df.book<Hist<1,float>>("x_y",100,0,100).fill(x).fill(y);
```

!!! warning "Make sure to get the returned booker"

    Reminder: each (chained) method returns a new node with the lazy action booked.
    In other words, make sure to obtain and use the returned aggregation for the columns to be actually filled!
    The following will be a mistake:
    ```cpp
    auto hist = df.book<Hist<1,float>>("x",50,0,400);
    hist.fill(x);  // mistake!
    ```
!!! tip "Actions on aggregations are "modular""
    An upside of the above caution is the modularity of actions that can be performed on aggregations.
    For example, a common binning of histograms for different variables can be re-cycled:
    ```cpp
    auto bins = df.book<Hist<1,float>>("hist",100,0,100);
    auto hist_x = bins.fill(x);
    auto hist_y = bins.fill(y);
    ```


