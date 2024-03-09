# Systematic variations

## Varying columns

Systematic variations are independent instances column nodes whose underlying data types are same as that of the nominal, but their values turn out to be different.
There are two ways to create a varied column: 

1. **Automatic.** Specify a specific type of column to be instantiated along with the nominal+variations. Always ensures the lockstep+transparent propagation of variations.
2. **Manual.** Provide existing instances of columns to be nominal+variations; any column whose output value type is compatible to that of the nominal can be set as a variation.

The two approaches can (and should) be used interchangeably with each other for full control over the creation & propagation of systematic variations through the dataflow.

=== "Automatic"

    | Column | Variation | Dependent? |
    | :--- | :--- | :--: |
    | `dataset::colum` | Read a different column of the same data type | N |
    | `column::constant` | Set a different value of the same data type | N |
    | `column::expression` | Use a different expression | Y |
    | `column::definition` | Initialize with a different constructor | Y |

    ### Independent columns

    === "Nominal"
        ```{.cpp .no-copy}
        auto x = ds.read(dataset::column<double>("x_nom"));
        ```
    === "Varied"
        ```{.cpp .no-copy}
        auto x = ds.vary(dataset::column<double>("x_nom"), 
            {"vary_x", "x_var"}, 
            {"vary_xy", "x_var"}
            );
        ```
    <!--  -->
    === "Nominal"
        ```{.cpp .no-copy}
        auto y = df.define(column::constant(1.0));
        ```
    === "Varied"
        ```{.cpp .no-copy}
        auto y = df.vary(column::constant(1.0), 
            {"vary_y", 2.0}, 
            {"vary_xy", 2.0}
            );
        ```
    !!! tip

        1. The "transparent" variations `"vary_x", "vary_y"` are declared in `x, y` respectively.
        2. The "lockstep" variation `"vary_xy"` is declared in both `x` and `y` columns.
        3. Then they are propagated *automatically* to `z` (see below).

    ### Dependent columns
    Variations for dependent columns have two points of entry:

    1. Its own definition, and
    2. Input columns.

    The varied definition will contain the propagated variations from both.
    === "Varied by input columns only"
        ```{.cpp .no-copy}
        auto z = x+y;
        ```
    === "Varied by definition + input columns"
        ```{.cpp .no-copy}
        auto z = df.vary(
            column::expression([](float x, float y){return x+y;}),
            systematic::variation("vary_zdef", [](double x, double y){return x-y;})
        )(x, y);                                  //(1)!
        ```

        1. Constrcutor arguments need not be the same type, as long as they are all valid for the column.

=== "Manual"

    ```{.cpp .no-copy}
    auto x_nom = ds.read(dataset::column<double>("x_nom"));
    auto x_var = ds.read(dataset::column<double>("x_var"));

    auto y_nom = ds.read(dataset::column<double>("y_nom"));
    auto y_var = ds.read(dataset::column<double>("y_var"));

    auto z = systematic::vary(
        systematic::nominal(x_nom+y_nom);
        systematic::variation("vary_x" x_var+y_nom);
        systematic::variation("vary_y" x_var+y_nom);
        systematic::variation("vary_xy" x_var+y_nom);
        systematic::variation("vary_zdef" x_var-y_nom);
    );
    ```

    !!! tip

        1. All variations are put in *manually* using existing columns as the user sees fit.
        2. The varied node propagates through automatically again afterwards. 
        
        

## Accessing the nominal & variations

```{.cpp .no-copy}
x.get_variation_names();                // {"vary_x","vary_xy"}
y.get_variation_names();                // {"vary_y","vary_xy"}
systematic::get_variation_names(x, y);  // {"vary_x","vary_y","vary_xy"}
z.get_variation_names();                // {"vary_x","vary_y","vary_xy","vary_zdef"}

x.has_variation("vary_x");   // true
x.has_variation("vary_y");   // false
x.has_variation("vary_xy");  // true

y.has_variation("vary_x");   // false
y.has_variation("vary_y");   // true
y.has_variation("vary_xy");  // true

z.has_variation("vary_x");     // true
z.has_variation("vary_y");     // true
z.has_variation("vary_xy");    // true
z.has_variation("vary_zdef");  // true

auto z_nom    = z.nominal();               // x_nom+y_nom
auto z_xvar   = z.variation("vary_x");     // x_var+y+nom
auto z_yvar   = z.variation("vary_y");     // x_nom+y_var
auto z_xyvar  = z.variation("vary_xy");    // x_var+y_var
auto z_defvar = z.variation("vary_zdef");  // x_nom-y_nom
```

## Using varied columns in selections/queries
```{.cpp .no-copy}
auto x = ds.vary(dataset::colum<double>("x_nom"), {"vary_x", "x_var"}); //(1)!
auto w = ds.read(dataset::column<double>("w")); //(2)!
auto q = df.make(query::plan<hist_1d>(lin_ax(100,0,1.0)).fill(x).book(w); //(3)!
```

1. I am varied.
2. I am not.
2. All actions, whether varied or not, work together.

!!! info
    Note the interoperability of a varied `#!cpp x` with a non-varied `#!cpp w`. 
    The same applies for varied selections and queries, i.e. all varied actions provide the same functionality as their regular counterparts.

## Accessing varied results
The index operator can be used as a shortcut for `#!cpp q.variation("var_name");` to retrieve individual results of a varied query:
```{.cpp .no-copy}
auto h_nom  = q.nominal().result();  // filled with "x_nom", weighted with "w"
auto h_xvar = q["vary_x"].result();  // filled with "x_var", weighted with "w"
```