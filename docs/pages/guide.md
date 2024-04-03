@page guide User guide
@tableofcontents

@section guide-dataflow Dataflow

@cpp
#include "queryosity.h"

using dataflow = qty::dataflow;
namespace multithread = qty::multithread;
namespace dataset = qty::dataset;
namespace column = qty::column;
namespace query = qty::query;
namespace systematic = qty::systematic;

int main() {

  dataflow df;

  // your analysis here...

}
@endcpp

The dataflow accepts several options to configure the dataset processing. 
Up to three keyword arguments can be provided in any order.

@cpp
// multithreaded run with a custom weight over the first 100 entries
dataflow df(multithread::enable(), dataset::weight(1.234), dataset::head(100));
@endcpp

| Option | Description | Default |
| :--- | :--- | :--- |
| `multithread::enable(nthreads)` | Enable multithreading up to `nthreads`. | `-1` (system maximum) |
| `multithread::disable()` | Disable multithreading. | |
| `dataset::weight(scale)` | Apply a global `scale` to all weights. | `1.0` |
| `dataset::head(nrows)` | Process the first `nrows` of the dataset. | `-1` (all entries) |

@section guide-dataset-reader Reading-in dataset(s)

Call queryosity::dataflow::load() with an input dataset and its constructor arguments.
The loaded dataset can then read out columns, provided their data types and names.

@cpp
std::ifstream data_json("data.json");

// load a dataset
using json = qty::json;
auto ds = df.load(dataset::input<json>(data_json));

// read a column
auto x = ds.read(dataset::column<double>("x"));

// shortcut: read multiple columns from a dataset
auto [w, cat] = ds.read(dataset::column<double>("weight"), dataset::column<std::string>("category"));
@endcpp

A dataflow can load multiple datasets, as long as all valid partitions reported by queryosity::dataset::source::partition() have the same number of total entries.
A dataset can report an empty partition, which signals that it relinquishes the control to the other datasets.

@cpp
std::ifstream data_csv("data.csv");

// another shortcut: load dataset & read column(s) at once
using csv = qty::csv;
auto y = df.read(dataset::input<csv>(data_csv), dataset::column<double>("y"));

// x from json, y from csv
auto z = x + y; // (see next section)
@endcpp

@see 
- queryosity::dataset::input (API)
  - queryosity::dataset::source (ABC)
  - queryosity::dataset::reader (ABC)
  - queryosity::json (Extension)
  - queryosity::csv (Extension)
- queryosity::dataset::column (API)
  - queryosity::column::reader (ABC)
  - queryosity::json::item (Extension)
  - queryosity::csv::cell (Extension)

@section guide-column Computing quantities

New columns can be computed out of existing ones by calling queryosity::dataflow::define() with the appropriate argument, or operators between the underlying data types.

@cpp
// constants columns do not change per-entry
auto zero = df.define(column::constant(0));
auto one = df.define(column::constant(1));
auto two = df.define(column::constant(2));

// binary/unary operators
auto three = one + two;
auto v_0 = v[zero];
// reminder: actions are *lazy*, i.e. no undefined behaviour (yet)

// self-assignment operators are not possible
// one += two;

// C++ function, functor, lambda, etc. evaluated out of input columns
// tip: pass large values by const& to prevent copies
auto s_length = df.define(
    column::expression([](const std::string &txt) { return txt.length(); }))(s);
@endcpp

A column can also be computed through a custom definition (see @ref example-stirling for an example), which enables full control over

- Customization: user-defined constructor arguments and member variables/functions.
- Optimization: the computation of each input column is deferred until value is invoked.

@see 
- queryosity::column::definition (API)
  - queryosity::column::definition<Out(Ins...)> (ABC)
- queryosity::column::observable (API)

@attention
Dependent column definitions must satisfy the following requirements: 
- Each instance must be thread-safe.
- Its custom constructor arguments must be CopyConstructible.
- Its output value must be DefaultConstructible and (CopyAssignable or MoveAssignable).

@section guide-selection Applying selections

Call queryosity::dataflow::filter() or queryosity::dataflow::weight() to initiate a selection in the cutflow, and apply subsequent selections from existing nodes to compound them. 

@cpp
auto [w, cat] = ds.read(dataset::column<double>("weight"),
                        dataset::column<std::string>("category"));
auto a = df.define(column::constant<std::string>("a"));
auto b = df.define(column::constant<std::string>("b"));
auto c = df.define(column::constant<std::string>("c"));

// initiate a cutflow
auto weighted = df.weight(w);

// cuts and weights can be compounded in any order.
auto cut =
    weighted.filter(column::expression([](double w) { return (w >= 0;); }))(w);

// applying more than one selection from a node creates a branching point.
auto cut_a = cut.filter(cat == a);
auto cut_b = cut.filter(cat == b);
auto cut_c = cut.filter(cat == c);

// selections can be merged based on their decision values.
auto cut_a_and_b = df.filter(cut_a && cut_b);
auto cut_b_or_c = df.filter(cut_b || cut_c);
@endcpp

@section guide-query Making queries

Call queryosity::dataflow::get() specifying the exact definition and constructor arguments of the query.
Subsequently, it can be filled with input columns and booked at a selection to instantiate the query:

@cpp
using h1d = qty::hist::hist<double>;
using h2d = qty::hist::hist<double,double>;
using linax = qty::hist::axis::regular;

auto q = df.get(query::output<h1d>(linax(100,0.0,1.0))).fill(x).at(cut);
@endcpp

A query can be filled multiple times, as long as the dimensionality of each call is appropriate:

@cpp
// fill 1d histogram with x & y for each entry
auto q_1xy = df.get(query::output<h1d>(linax(100,0.0,1.0))).fill(x).fill(y).at(cut);

// fill 2d histogram with (x, y) for each entry
auto q_2xy = df.get(query::output<h2d>(linax(100,0.0,1.0), linax(100,0.0,1.0))).fill(x, y).at(cut);
@endcpp

Multiple queries can be instantiated at once by:

1. Making a specific query at multiple selections.
2. Making multiple (different) queries from a selection.

@cpp
// 1.
auto [q_1xy_a, q_1xy_b] = q_1xy.book(cut_a, cut_b);

// 2.
auto [q_1xy_c, q_2xy_c] = c.book(q_1xy, q_2xy);
@endcpp

Access the result of a query to turn all actions eager.

@cpp
auto h1x_a = q_1xy_a.result(); // takes a while
auto h2xy_c = q_2xy_c.result(); // instantaneous
@endcpp

@see 
- queryosity::query::output (API)
- queryosity::query::definition<Out(Obs...)> (ABC)
- queryosity::hist::hist (Extension)

@section guide-vary Systematic variations

Specifying systematic variations on a column is as simple as it can be: provide the nominal argument and a mapping of variation name to alternate arguments to queryosity::dataflow::vary() in lieu of the usual queryosity::dataflow::define().

@cpp
// dataset columns are varied by different column names from the loaded dataset
auto x = ds.vary(dataset::column<double>("x_nom"),
                 {{"shift_x", "x_shifted"}, {"smear_x", "x_smeared"}});

// constants are varied by alternate values
auto pm1 = df.vary(column::constant(0), {{"plus_1", 1}, {"minus_1", -1}});

// dependent columns are varied by alternate constructor argument(s)
// and input columns
auto x =
    df.vary(column::expression([](float x, float y) { return x + y; }),
            {{"kill_x", [](double x, double y) { return x * y; }}})(x, pm1);
@endcpp

Alternatively, provide existing instances of columns to be the nominal and its variations, where column whose data type is compatible with that of the nominal can be set as a variation.

@cpp
// note: different column types of different data types!
auto z_nom = ds.read(dataset::column<double>("z"));
auto z_fixed = df.define(column::constant<int>(100.0));
auto z_half =
    df.define(column::expression([](float z) { return z * 0.5; }))(z_nom);
auto z = systematic::vary(systematic::nominal(z_nom),
                          systematic::variation("z_fixed", f_fixed),
                          systematic::variation("z_half", z_half));
@endcpp

The set of variations active in nodes can be checked as they are propagated through the dataflow.
After the dust settles, the nominal and each varied result of a query can be accessed individually.

@cpp
// check variations
x.has_variation("shift_x"); // true, x_shifted + 0
x.has_variation("smear_x"); // true, x_smeared + 0
x.has_variation("plus_1");  // true, x_nom + 1
x.has_variation("minus_1"); // true, x_nom - 1
x.has_variation("kill_x");  // true: x_nom * 0
x.has_variation("no"); // false

// propagation through selection and query
auto yn = df.vary(column::constant(true), {{"no", false}});

systematic::get_variation_names(
    x, yn); // {"shift_x", "smear_x", "plus_1", "minus_1", "kill_x", "no"}

auto cut = df.filter(yn);
auto q = df.get(column::series(x)).book(cut);

q.get_variation_names(); // {"shift_x", "smear_x", "plus_1", "minus_1",
                         // "kill_x", "no"}

// access nominal+variation results
q.nominal().result();   // {x_nom_0, ..., x_nom_N}
q["shift_x"].result(); // {x_shifted_0, ..., x_shifted_N}
q["kill_x"].result();  // {0, ..., 0}
q["no"].result();      // {}
@endcpp

@see 
- queryosity::lazy::varied
- queryosity::todo::varied