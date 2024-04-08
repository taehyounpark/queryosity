# Systematic variations

To specifying systematic variations on a column, provide the nominal argument and a mapping of variation name to alternate arguments to queryosity::dataflow::vary() instead of the usual queryosity::dataflow::define().

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
                 {{"shift_x", "x_shifted"}, {"smear_x", "x_smeared"}});
```
:::
::::

::::{tab-set}
:::{tab-item} Constant
:sync: nom
```{code} cpp 
auto zero = df.define(column::constant(0));
```
:::
:::{tab-item} Varied
:sync: var
```{code} cpp 
auto pm1 = df.vary(column::constant(0), {{"plus_1", 1}, {"minus_1", -1}});
```
:::
::::

::::{tab-set}
:::{tab-item} Expression
:sync: nom
```cpp
auto x = df.define(column::constant(0));
```
:::
:::{tab-item} Varied
:sync: var
```cpp
auto x =
    df.vary(column::expression([](float x, float y) { return x + y; }),
            {{"kill_x", [](double x, double y) { return x * y; }}})(x, pm1);
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
auto z = systematic::vary(systematic::nominal(z_nom),
                          systematic::variation("z_fixed", f_fixed),
                          systematic::variation("z_half", z_half));
```
:::
::::

## Checking variations

The set of variations active in a lazy action can be checked as they are propagated through the dataflow by:

```cpp
// check variations
x.has_variation("shift_x"); // true, x_shifted + 0
x.has_variation("smear_x"); // true, x_smeared + 0
x.has_variation("plus_1");  // true, x_nom + 1
x.has_variation("minus_1"); // true, x_nom - 1
x.has_variation("kill_x");  // true: x_nom * 0
x.has_variation("no");      // false

// propagation through selection and query
auto yn = df.vary(column::constant(true), {{"no", false}});

systematic::get_variation_names(
    x, yn); // {"shift_x", "smear_x", "plus_1", "minus_1", "kill_x", "no"}

auto cut = df.filter(yn);
auto q = df.get(column::series(x)).at(cut);

q.get_variation_names(); // same set as (x, yn) above
```

## Accessing varied results

```cpp
// access nominal+variation results
q.nominal().result();  // {x_nom_0, ..., x_nom_N}
q["shift_x"].result(); // {x_shifted_0, ..., x_shifted_N}
q["plus_1"].result();  // {x_nom_0 + 1, ..., x_nom_N + 1}
q["kill_x"].result();  // {0, ..., 0}
q["no"].result();      // {}
```

```cpp
auto q_results = systematic::get_results(q);

q_results.nominal;    // {x_nom_0, ..., x_nom_N}
q_results["shift_x"]; // {x_shifted_0, ..., x_shifted_N}
q_results["plus_1"];  // {x_nom_0 + 1, ..., x_nom_N + 1}
q_results["kill_x"];  // {0, ..., 0}
q_results["no"];      // {}
```