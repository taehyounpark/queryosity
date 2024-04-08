# Column as a series

```cpp
// single column
// (sel: (varied) lazy selection)
auto x = ds.read(dataset::column<double>("x"));
auto x_arr = df.get(column::series(x)).at(sel).result(); // std::vector<double>

// single column at multiple selections
// (sel_a/b/c: (varied) lazy selections)
auto [x_arr_a, x_arr_b, x_arr_c] = df.get(column::series(x)).at(sel_a, sel_b, sel_c);

// multiple columns at a selection
auto y = ds.read(dataset::column<int>("y"));
auto z = ds.read(dataset::column<std::string>("z"));
auto [y_arr, z_arr] = sel.get(column::series(y, z));
```