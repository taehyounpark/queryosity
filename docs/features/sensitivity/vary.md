## Applying variations

The general recipe is as follows:

  1. Define the nominal column.
  2. Use it 

### Dataset column

Dataset columns are varied from the dataset by providing the variation name and alternative column name around brace initializers:

=== "Nominal column"
    ```cpp
    auto x = ds.read<float>("x_nom");
    ```
=== "Varied column name"
    ```cpp
    auto x = ds.vary( ds.read<float>("x_nom"), {"vary_x", "x_var"} );
    ```

### Constant

Similarly as above, constants can be varied from the dataflow:

=== "Nominal value"
    ```cpp
    auto y = df.constant<float>(20.0);
    ```
=== "Varied value"
    ```cpp
    auto y = ds.vary( df.constant(20.0), {"vary_y", 30.0} );
    ```

### Definition

Variations for column definitions have two points of entry:

  1. The first `define` call (expression/constructor).
  2. The second `evaluate` call (input columns).

In the first case, a `systematic::variation` wrapper around the variation name and definition is used:

=== "Varied by input columns only"
    ```cpp
    auto z = df.define([](float x, float y){return x+y;})(x, y);
    ```
=== "Varied by input columns + definition"
    ```cpp
    auto z = df.vary(
      df.define([](float x, float y){return x+y;}),
      systematic::variation("vary_zdef", [](float x, float y){return x-y;})
    )(x, y);
    ```


## Accessing variations

The existence of variations within a node can be checked as:
```cpp
auto x_var_names = x.list_variation_names();
x.has_variation("vary_x");  // true
x.has_variation("vary_y");  // false
```
The nominal and each variation node can be individually accessed as:
```cpp
auto x_nom = x.nominal();
auto x_var = x["vary_x"];
```