# Systematic variations

To create systematic variations of a column, substitute its `dataflow::define()` with `dataflow::vary()` and provide a mapping of variation name to alternate column definitions as a secondary argument.

## Varying columns

::::{tab-set}
:::{tab-item} Dataset column
:sync: nom
```{code} cpp 
auto x = ds.read(dataset::column<double>("x_nom"));
```
:::
:::{tab-item} Varied
:sync: var
```{code} cpp 
auto x = ds.vary(dataset::column<double>("x_nom"),
                 {{"x_up", "x_up"}, {"x_down", "x_dn"}});
```
:::
::::

::::{tab-set}
:::{tab-item} Constant
:sync: nom
```{code} cpp 
auto x = df.define(column::constant(0));
```
:::
:::{tab-item} Varied
:sync: var
```{code} cpp 
auto x = df.vary(column::constant(0), {{"plus_1", 1}, {"minus_1", -1}});
```
:::
::::

::::{tab-set}
:::{tab-item} Expression
:sync: nom
```cpp
auto z = df.define(column::expression([](float x, double y) { return x + y; })(x, y);
```
:::
:::{tab-item} Varied
:sync: var
```cpp
auto z =
    df.vary(column::expression([](float x, double y) { return x + y; }),
            {{"times", [](double x, float y) { return x * y; }}})(x, y);
```
:::
::::

::::{tab-set}
:::{tab-item} Definition
:sync: nom
```cpp
auto z = df.define(column::definition<DEF>(ARGS...))(x, y);
```
:::
:::{tab-item} Varied
:sync: var
```cpp
auto z = df.vary(column::definition<DEF>(ARGS_NOM...),
                 {{"var_name", {ARGS_VAR...}}})(x, y);
```
:::
::::

::::{tab-set}
:::{tab-item} Arbitrary columns
:sync: nom
```cpp
auto z_nom = ds.read(dataset::column<double>("z"));
auto z_fixed = df.define(column::constant<int>(100.0));
auto z_half =
    df.define(column::expression([](float z) { return z * 0.5; }))(z_nom);
```
:::
:::{tab-item} Varied
:sync: var
```cpp
auto z =
    df.vary(column::nominal(z_nom), {{"z_fixed", f_fixed}, {"z_half", z_half}});
```
:::
::::

## Checking variations

The set of variations active in a lazy action can be checked as they are propagated through the dataflow by:

```cpp
auto x = ds.vary(dataset::column<double>("x_nom"),
                 {{"x_up", "x_up"}, {"x_down", "x_dn"}});
auto yn = df.vary(column::constant(true), {{"no", false}});

// check variations
x.has_variation("x_up"); // true
x.has_variation("x_dn"); // true
x.has_variation("no");   // false

// helper function to get active systematic variations in a set of actions
systematic::get_variation_names(x, yn); // {"x_up", "x_dn", "no"}

// systematic variations get propagated through selections & queries
auto cut = df.filter(yn);
auto q = df.get(column::series(x)).at(cut);
q.get_variation_names(); // {"x_up", "x_dn", "no"}
```

## Accessing varied results

```cpp
// access nominal+variation queries and their results
q.nominal().result(); // {x_nom_0, ..., x_nom_N}
q["x_up"].result();   // {x_up_0, ..., x_up_N}
q["x_dn"].result();   // {x_dn_0, ..., x_dn_N}
q["no"].result();     // {}

// alternatively, use this helper function to retrieve the results
auto q_results = systematic::get_results(q);
q_results.nominal;
q_results["x_up"];
q_results["x_dn"];
q_results["no"];
```