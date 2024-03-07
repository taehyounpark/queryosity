# Systematic variations

## Applying variations

There are two ways to apply systematic variations on a column:

1. **Simultaneous.** Specify the concrete column of the nominal along with its variations.
2. **Individual.** Define the nominal and variation as separate columns, bring them together under a varied column.

There is no difference in the logical outcome of the systematic variations between the two methods: which method is better for the analyzer depends on how they wish to expression the computation graph.

### Simultaneous

This method provides the cleanest way to specify the nominal and variations of a given column type, which translates to:

| Column | Variation | Dependent? |
| :--- | :--- | :--: |
| `reader` | Read a different column of the same data type | N |
| `constant` | Set a different value of the same data type | N |
| `equation` | Use a different expression | Y |
| `definition` | Initialize with a different constructor | Y |

#### Independent columns

Variations of independent columns whose values do not depend on others can be varied as:

=== "Nominal"
    ```cpp
    auto x = ds.read( dataset::column<double>("x_nom") );
    ```
=== "Varied"
    ```cpp
    auto x = ds.vary( dataset::column<double>("x_nom"), {"vary_x", "x_var"} );
    ```

<!--  -->

=== "Nominal"
    ```cpp
    auto y = df.define( column::constant(1.0) );
    ```
=== "Varied"
    ```cpp
    auto y = df.vary( column::constant(1.0), {"y_up", 2.0}, {"y_dn", 0.5} );
    ```

#### Dependent columns

Variations for dependent columns have two points of entry:

  1. Its own definition.
  2. Input columns, any of which may or may have been be varied.

=== "Varied by input columns only"
    ```cpp
    auto z = df.define([](float x, float y){return x+y;})(x, y);
    ```
=== "Varied by definition + input columns"
    ```cpp
    auto z = df.vary(
      column::expression([](float x, float y){return x+y;}),
      systematic::variation("vary_zdef", [](float x, float y){return x-y;})
    )(x, y);
    ```

### Individual

This method provides the most general way possible to specify systematic variations: *any column can be varied by another*, as long as their underlying data types are compatible (up to conversion).

(To do)

## Accessing the nominal and variations of a varied action

As mentioned earlier, once an action is varied, its propagation through the rest of the dataflow occurs automatically.
The nominal and each variation node can be individually accessed as:
```cpp
auto x = ds.vary( dataset::column<double>("x_nom"), {"vary_x", "x_var"} );
auto x_nom = x.nominal();  
auto x_var = x.variation("x_var");
```
The existence of variations within a node can be checked as:
```cpp
auto x_var_names = x.list_variation_names();  // {"vary_x"}
x.has_variation("vary_x");  // true
x.has_variation("vary_y");  // false
```
In accessing query results, the index operator acts as a shortcut to retrieve the varied result:
```cpp
auto w = df.vary( column::constant<double>(1), {"vary_w", 2.0} );
auto h = df.make( query::plan<h1d>(100,0,1.0) ).fill(x).book(w);

auto h_nom = h.nominal().result();
auto h_xvar = h["vary_x"].result();
auto h_wvar = h["vary_w"].result();
```