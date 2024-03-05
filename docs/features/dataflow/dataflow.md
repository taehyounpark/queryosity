# The `dataflow` object

The dataflow object can be instantiated by:

```cpp
qty::dataflow df;
```
There are several "keyword" arguments (can be provided in any order) available to configure the dataset processing:

| Keyword | Argument | Description |
| :--- | :--- | :--- |
| `qty::multithread::enable(nthreads)` | `nthreads` | Enable multithreading up to specified thread count. |
| `qty::multithread::disable()` | | Disable multithreading. |
| `qty::dataset::head(nrows)` | `nrows` | Process only the first N row(s) of the dataset. |
| `qty::dataset::weight(scale)` | `scale` | Apply a global scale to all weights. |

!!! example
    ```cpp
    qty::dataflow df( 
        qty::multithread::enable(4), 
        qty::dataset::limit(100), 
        qty::dataset::weight(0.123)
        );
    ```