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

@section guide-dataset-reader Reading a dataset

Call queryosity::dataflow::load() with an input dataset and its constructor arguments.
The loaded dataset can then read out columns, provided their data types and names.

@cpp
using json = qty::json;

// load a dataset
std::ifstream data("data.json");
auto ds = df.load(dataset::input<json>(data));

// read a dataset column
auto x = ds.read(dataset::column<double>("x"));
@endcpp

A dataflow can load multiple datasets, as long as all valid partitions reported by queryosity::dataset::source::partition() have the same number of total entries.
Or, a dataset can report an empty partition, which signals that it relinquishes the control to the other datasets.

@cpp
// no need to be another json -- whatever else!
std::ifstream more_data("more_data.json");
auto ds_another = df.load(dataset::input<json>(more_data));

// no need to be another double -- whatever else!
auto y = ds_another.read(dataset::column<double>("y"));

// shortcut: implicitly load a dataset and read out all columns at once.
std::ifstream even_more_data("even_more_data.json");
auto [s, v] = df.read(
  dataset::input<json>(even_more_data),
  dataset::column<std::string>("s"),
  dataset::column<std::vector<double>>("v")
  );
@endcpp

@see 
- queryosity::dataset::input (API)
  - queryosity::dataset::source (ABC)
  - queryosity::dataset::reader (ABC)
  - queryosity::json (Extension)
- queryosity::dataset::column (API)
  - queryosity::column::reader (ABC)
  - queryosity::json::item (Extension)

@section guide-column Computing quantities

Call queryosity::dataflow::define() with the appropriate argument.

@cpp
// -----------------------------------------------------------------------------
// constant
// -----------------------------------------------------------------------------
// their values will not change per-entry

auto zero = df.define(column::constant(0));
auto one = df.define(column::constant(1));
auto two = df.define(column::constant(2));

// -----------------------------------------------------------------------------
// simple expression
// -----------------------------------------------------------------------------
// binary/unary operators between underlying data types

auto three = one + two;
auto v0 = v[zero];

// one += zero; // self-assignment operators are not possible

// -----------------------------------------------------------------------------
// custom expression
// -----------------------------------------------------------------------------
// (C++ funciton, functor, lambda, etc.) evaluated out of input columns

// pass large values by const reference to prevent expensive copies
auto s_length = df.define(column::expression([](const std::string& txt){return txt.length();}), s);

// -----------------------------------------------------------------------------
// custom definition
// -----------------------------------------------------------------------------
// the most general & performant way to compute a column

class VectorSelection : public column::definition<> 
{

};
auto v_selected = df.define(column::definition<>(), v);
@endcpp

@see 
- queryosity::column::definition (API)
  - queryosity::column::definition<Out(Ins...)> (ABC)

@section guide-selection Applying selections

Call queryosity::dataflow::filter() or queryosity::dataflow::weight() to initiate a selection in the cutflow, and apply subsequent selections from existing nodes to compound them. 

@cpp
// initiate a cutflow 
auto inclusive = df.filter(c); // using an existing column as the decision

// selections can be compounded regardless of their type (cut or weight)
auto weighted = cut.weight(
  column::expression([](double w){return (w<0 ? 0.0: w);}), w
  ); // using an exprssion evaluated out of input columns

// compounding multiple selections from a common node creates a branching point
auto cut_a = weighted.filter(a);
auto cut_b = weighted.filter(b);
auto cut_c = weighted.filter(c);

// selections are columns whose values are their decisions along the cutflow,
// which can also be used to evaluate new selections.
auto cut_a_and_b = df.filter(cut_a && cut_b);
auto cut_b_or_c = df.filter(cut_b || cut_c);
@endcpp

@section guide-query Making queries

Call queryosity::dataflow::make() with a "plan" specifying the exact definition and constructor arguments of the qeury.
Subsequently, the plan can be filled with input columns and booked at a selection to instantiate the query.

@cpp
using h1d = qty::hist::hist<double>;
using h2d = qty::hist::hist<double,double>;
using linax = qty::hist::axis::linear;

// instantiate a 1d histogram query filled with x over all entries
auto q = df.make(query::plan<h1d>(linax(100,0.0,1.0))).fill(x).book(inclusive);

// a plan can book multiple queries at a selection
auto [q_a, q_b] = df.make(query::plan<h1d>(linax(100,0.0,1.0))).fill(x).book(cut_a, cut_b);

// a selection can book multiple queries (of different types)
auto qp_1d = df.make(query::plan<h1d>(linax(100,0.0,1.0))).fill(x);
auto qp_2d = df.make(query::plan<hist2d>(linax(100,0.0,1.0),linax(100,0.0,1.0))).fill(x,y);
auto [q_1d, q_2d] = sel.book(qp_1d, qp_2d);
@endcpp

Access the result of a query to turn all actions eager.

@cpp
auto hx_a = q_a.result();  // takes a while -- dataset traversal
auto hxy_sel = q_2d.result();  // instantaneous -- already completed
@endcpp

@see 
- queryosity::query::plan (API)
- queryosity::query::definition<T(Obs...)> (ABC)
- queryosity::hist::hist (Extension)

@section guide-vary Systematic variations

Call queryosity::dataflow::vary() to create varied columns. 
There are two ways in which variations can be specified:

1. **Automatic.** Specify a specific type of column to be instantiated along with the nominal+variations. Always ensures the lockstep+transparent propagation of variations.
2. **Manual.** Provide existing instances of columns to be nominal+variations; any column whose output value type is compatible to that of the nominal can be set as a variation.

Both approaches can (and should) be used in a dataflow for full control over the creation & propagation of systematic variations.

@cpp
// automatic -- set and forget

// dataset columns are varied by different column names
auto x = ds.vary(
  dataset::column("x_nom"),
  {"vary_x","x_var"}
  );

// constants are varied by alternate values
auto yes_or_no = df.vary(
  column::constant(true),
  {"no", false}
  );

// expressions are varied by alternate expression+input columns
auto b = df.vary(
  column::expression(),
  systematic::variation("vary_b", )
  )();

// definitions are varied by alternate constructor arguments+input columns
auto defn = df.vary(
  column::definition<>(),
  systematic::variation("vary_c", )
  )();

// manual method -- man-handle as you see fit

// note:
// - different column types (column, constant, definition)
// - different data types (double, int, float)
auto z_nom = ds.read(dataset::column<double>("z"));
auto z_fixed = df.define(column::constant<int>(100.0));
auto z_half = df.define(column::expression([](float z){return z*0.5;}), z_nom);

// as long as their output values are compatible, any set of columns can be used
auto z = systematic::vary(
  systematic::nominal(z_nom), 
  systematic::variation("z_fixed", f_fixed), 
  systematic::variation("z_half", z_half)
  );

// systematic variations are propagated through selections...
auto sel = df.filter(yes_or_no);

// ... and queries
auto q = df.make(query::plan<>()).fill(x).book(yes_or_no);
q.has_variation("vary_x"); // true, filled with x_var for all entries
q.has_variation("no"); // true, filled with x_nom for no entries

// other variations play no role here.
q.has_variation("vary_b"); // false

// nominal & varied results can be separately accessed
auto q_nom_res = q.nominal().result();
auto q_varx_res = q.["vary_x"].result();
auto q_none_res = q.["no"].result();
@endcpp

@see 
- queryosity::lazy::varied
- queryosity::todo::varied