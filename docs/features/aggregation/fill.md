An aggregation can be filled with column values by the `fill()` method:
```cpp
auto hist = df.book<Hist<1,float>>("x",100,0,100).fill(x);
```
The arguments must match the dimensionality of the implemented aggregation:
```cpp
auto hist_xy = df.book<Hist<2,float>>("xy",100,0,100,100,0,100).fill(x,y);
```
As long as that is the case, an aggregation can be filled any number of times:
```cpp
auto hist_x_y = df.book<Hist<1,float>>("x_y",100,0,100).fill(x).fill(y);
```
!!! warning "Make sure to get the returned booker"
    Under the hood, the method chaining works by returning another aggregation independent instance, while the former will remain unchanged.
    So make sure to obtain the returned aggregation for the columns to be actually filled!
    The following will be a mistake:
    ```cpp
    auto hist = df.book<Hist<1,float>>("x",50,0,400);
    hist.fill(x);  // <- mistake!
    ```
!!! tip "Actions on aggregations are "modular""
    An upside of the above caution is the modularity of actions that can be performed on aggregations.
    For example, a common binning of histograms for different variables can be re-cycled:
    ```cpp
    auto bins = df.book<Hist<1,float>>("hist",100,0,100);
    auto hist_x = bins.fill(x);
    auto hist_y = bins.fill(y);
    ```


