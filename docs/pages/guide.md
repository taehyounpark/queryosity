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
    column::expression([](const std::string &txt) { return txt.length(); }), s);
@endcpp

A column can also be defined by a custom implementation, which offers:

- Customization: user-defined constructor arguments and member variables/functions.
- Optimization: each input column is provided as a column::observable<T>, which defers its computation for the entry until column::observable<T>::value() is invoked.

As an example, consider the following calculation of a factorial via Stirling's approximation:

@cpp
using ull_t = unsigned long long;

// 1. full calculation
auto factorial(ull_t n) {
  ull_t result = 1;
  while (n > 1)
    result *= n--;
  return result;
}
// 2. approximation
auto stirling = []() {
  return std::round(std::sqrt(2 * M_PI * n) * std::pow(n / std::exp(1), n));
};

// 1. if n is small enough to for n! to fit inside ull_t, use full calculation
// 2. if n is large enough, use approximation
auto n = ds.read(dataset::column<ull_t>("n"));
auto n_f_fast = df.define(column::expression(stirling), n);
auto n_f_full = df.define(column::expression(factorial), n);
ull_t n_threshold = 10;

// using expression
auto n_f_slow = df.define(column::expression([n_threshold](ull_t n, ull_t fast, ull_t slow){
  return n >= n_threshold ? n_fast : n_flow; });
// time elapsed = t(n) + t(fast) + t(slow)
// :(

// using definition
class Factorial : public column::definition<double(ull_t, double, double)> {
public:
  Factorial(ull_t threshold) : m_threshold(threshold) {}
  virtual ~Factorial() = default;
  ull_t evaluate(column::observable<ull_t> n, column::observable<ull_t> fast,
                 column::observable<ull_t> full) const override {
    return (n.value() >= m_threshold) ? fast.value() : full.value();
  }
  void adjust_threshold(ull_t threshold) {
    m_threshold = threshold;
  }

protected:
  ull_t m_threshold;
};
auto n_f_best = df.define(column::definition<Factorial>(n_threshold), n, n_f_fast, n_f_full);
// time elapsed = t(n) + { t(n_fast) if n >= 10, t(n_slow) if n < 10 }
// :)

// advanced: access per-thread instance
dataflow::node::invoke([](Factorial* n_f){n_f->adjust_threshold(20);}, n_f_best);
@endcpp

@see 
- queryosity::column::definition (API)
  - queryosity::column::definition<Out(Ins...)> (ABC)
- queryosity::column::observable<T> (API)

@section guide-selection Applying selections

Call queryosity::dataflow::filter() or queryosity::dataflow::weight() to initiate a selection in the cutflow, and apply subsequent selections from existing nodes to compound them. 

@cpp
auto [w, cat] = ds.read(dataset::column<double>("weight"), dataset::column<std::string>("category"));
auto a = df.define(column::constant<std::string>("a"));
auto b = df.define(column::constant<std::string>("b"));
auto c = df.define(column::constant<std::string>("c"));

// initiate a cutflow 
auto weighted = df.weight(w);

// cuts and weights can be compounded in any order.
auto cut = weighted.filter(
  column::expression([](double w){return (w>=0;);}), w
  );

// applying more than one selection from a node creates a branching point.
auto cut_a = cut.filter(cat == a);
auto cut_b = cut.filter(cat == b);
auto cut_c = cut.filter(cat == c);

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

// plan a 1d/2d histogram query filled with x/(x,y)
auto q_1d = df.get(query::output<h1d>(linax(100,0.0,1.0))).fill(x);
auto q_2d = df.get(query::output<h2d>(linax(100,0.0,1.0), linax(100,0.0,1.0))).fill(x,y);

// query at multiple selections
auto [q_1d_a, q_1d_b] = q_1d.book(cut_a, cut_b);

// multiple (different) queries at a selection
auto [q_1d_c, q_2d_c] = c.book(q_1d, q_2d);
@endcpp

Access the result of a query to turn all actions eager.

@cpp
auto hx_a = q_1d_a.result(); // takes a while
auto hxy_c = q_2d_c.result(); // instantaneous
@endcpp

@see 
- queryosity::query::output (API)
- queryosity::query::definition<T(Obs...)> (ABC)
- queryosity::hist::hist (Extension)

@section guide-vary Systematic variations

Call queryosity::dataflow::vary() to create varied columns. 
There are two ways in which variations can be specified:

1. **Pre-instantiation.** Provide the nominal argument and a mapping of variation name to alternate arguments.
2. **Post-instantiation.** Provide existing instances of columns to be the nominal and its variations.
  - Any column whose data type is compatible with that of the nominal can be set as a variation.

Both approaches can (and should) be used in a dataflow for full control over the creation & propagation of systematic variations.

@cpp
// pre-instantiation

// dataset columns are varied by different column names
auto x = ds.vary(
  dataset::column<double>("x_nom"),
  {{"shift_x","x_shifted"}, {"smear_x", "x_smeared"}}
  );

// constants are varied by alternate values
auto y = df.vary(
  column::constant(true),
  {{"no", false}}
  ); // qty::lazy<column::fixed<double>>::varied
// (column::fixed<dobule> is the concrete type)

// expressions are varied by alternate expression and input columns
auto x_pm_y = df.vary(
  column::expression([](double x, float y){return x+y;}),
  {
    {"minus_y", [](double x, float y){return x-y;}}
  }
  )(x, y);

// post-instantiation

// note:
// - different column types (column, constant, definition)
// - different data types (double, int, float)
auto z_nom = ds.read(dataset::column<double>("z"));
auto z_fixed = df.define(column::constant<int>(100.0));
auto z_half = df.define(column::expression([](float z){return z*0.5;}), z_nom);
auto z = systematic::vary(
  systematic::nominal(z_nom), 
  systematic::variation("z_fixed", f_fixed), 
  systematic::variation("z_half", z_half)
  ); // qty::lazy<column::valued<double>>::varied
// (column::valued<double> is the common denominator base class)
@endcpp

The list of variations in a (set of) action(s) can be always be checked as they are propagated through the dataflow.
After the dust settles, the nominal and each varied result of a query can be accessed individually.

@cpp
// check variations
x.has_variation("shift_x"); // true
x.has_variation("smear_x"); // true
x.has_variation("no"); // false

yes_or_no.has_variation("shift_x"); // false
yes_or_no.has_variation("smear_x"); // false
yes_or_no.has_variation("no"); // true

systematic::get_variation_names(x, yes_or_no); // {"shift_x", "smear_x", "no"}

// propagation through selection and query
auto sel = df.filter(yes_or_no);
auto q = df.get(column::series(x)).book(sel);

q.has_variation("shift_x"); // true
q.has_variation("smear_x"); // true
q.has_variation("no"); // true

// access nominal+variation results
q.nominal().result(); // {x_nom_0, ..., x_nom_N}
q.["shift_x"].result(); // {x_shifted_0, ..., x_shifted_N}
q.["smear_x"].result(); // {x_smeared_0, ..., x_smeared_N}
q.["no"].result(); // {}
@endcpp

@see 
- queryosity::lazy::varied
- queryosity::todo::varied