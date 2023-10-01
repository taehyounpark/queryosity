# Booking selections

Similar in spirit to filling columns, an aggregation can be booked at multiple selections:

=== "One selection"
    ```cpp
    hist_x = df.book<hist::hist<float>>(hist::axis::regular(100,0,100))\
              .fill(x)\
              .at(cut_inclusive);
    ```
=== "Multiple selections"
    ```cpp
    hist_x = df.book<hist::hist<float>>(hist::axis::regular(100,0,100))\
              .fill(x)\
              .at(cut_inclusive, weighted_entries, region_a);
    ```