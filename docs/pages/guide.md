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
using json = qty::json;

std::ifstream data("data.json");
auto ds = df.load(dataset::input<json>(data));

auto x = ds.read(dataset::column<double>("x"));
@endcpp

A dataflow can load multiple datasets, as long as all valid partitions reported by queryosity::dataset::source::partition() have the same number of total entries.
A dataset can report an empty partition, which signals that it relinquishes the control to the other datasets.

@cpp
using csv = qty::csv;

std::ifstream data_csv("data.csv");
auto y = df.load(dataset::input<csv>(data_csv)).read(dataset::column<double>("y"));

auto z = x+y;
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
// -----------------------------------------------------------------------------
// initiate a cutflow 
// -----------------------------------------------------------------------------

// pass all entries, apply a weight
auto weighted = df.weight(w);

// -----------------------------------------------------------------------------
// compounding 
// -----------------------------------------------------------------------------
// cuts and weights can be compounded in any order.

// ignore entry if weight is negative
auto cut = weighted.filter(
  column::expression([](double w){return (w>=0;);}), w
  );

// -----------------------------------------------------------------------------
// branching out
// -----------------------------------------------------------------------------
// applying more than one selection from a node creates a branching point.

auto cat = ds.read<std::string>("cat");

auto a = df.define(column::constant<std::string>("a"));
auto b = df.define(column::constant<std::string>("b"));
auto c = df.define(column::constant<std::string>("c"));

auto cut_a = cut.filter(cat == a);
auto cut_b = cut.filter(cat == b);
auto cut_c = cut.filter(cat == c);

// -----------------------------------------------------------------------------
// merging
// -----------------------------------------------------------------------------
// selections can be merged based on their decision values.

auto cut_a_and_b = df.filter(cut_a && cut_b);
auto cut_b_or_c = df.filter(cut_b || cut_c);
@endcpp

@section guide-query Making queries

Call queryosity::dataflow::make() with a "plan" specifying the exact definition and constructor arguments of the qeury.
Subsequently, the plan can be filled with input columns and booked at a selection to instantiate the query.

@cpp
using h1d = qty::hist::hist<double>;
using h2d = qty::hist::hist<double,double>;
using linax = qty::hist::axis::regular;

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