# Computing quantities

::::{tab-set}

:::{tab-item} Constant
:::{card} Template
```cpp
auto cnst = df.define(dataset::constant(VAL));
```
:::

:::{tab-item} Expression
:::{card} Template
```cpp
auto eqn = df.define(column::expression(FUNC))(COLS...);
```
:::

:::{tab-item} Definition
:::{card} Template
```cpp
auto defn = df.define(column::definition<DEF>(ARGS...))(COLS...);
```
:::

::::

:::{admonition} Requirements on column value type
:class: important
A computed column **MUST** output a value of a type `T` that is:
- {{DefaultConstructible}}.
- {{CopyAssignable}} or {{MoveAssignable}}.
:::

(computing-columns-operators)=
## Basic operations

Binary and unary operators on the underlying value types are supported.

```cpp
// constants columns do not change per-entry
auto zero = df.define(column::constant(0));
auto one = df.define(column::constant(1));
auto two = df.define(column::constant(2));

// binary/unary operators
auto three = one + two;
auto v_0 = v[zero]; // no undefined behaviour (yet), even if v.size()==0.

// can be re-assigned as long as value type remains unchanged
two = three - one;
```

## Custom expressions

User must provie a C++ callable (function, labmda, etc.) with `column::observable<T>` arguments, for example: 

```cpp
auto s_length = df.define(
    column::expression([](column::observable<std::string> txt) { return txt.length(); }))(s);
```

:::{tip}
There are two important reasons behind the `column::observable<T>` requirement:

- Enforce that the user only has access to the value as a read-only reference (`const &`).
- The actual value of the argument is not actually computed until `.value()` is explicitly called!

The second point is especially important when e.g. there are expensive computation operations that the user may wish to defer on a case-by-case basis.
As still guaranteed across the global computation graph, any values previously computed are accessed without copying by virtue of the first point.
:::

## Custom definitions

A column can also be computed through a custom column definition, which mainly enables full control over its constructor arguments.
The user is free to implement any member variables and/or methods to the class of course, however their usage is primarily limited to the class' behviour during the entry loop since the user will not have direct access to the class instances, rather their lazy nodes.

:::{admonition} Thread-safety requirements
:class: caution
- Each instance of a custom definition operate in a thread-safe manner.
- Its constructor arguments must be {{CopyConstructible}}.
:::

Consider the following example of computing the factorial of a number, $ n! = 1 \times 2 \times 3 \times ... \times n $:

1. if $ n! $ is too small and/or can fit inside `unsigned long long`, use full calculation.
2. if $ n $ is large enough, use Stirling's approximation.

```cpp
#include <fstream>
#include <sstream>
#include <vector>

#include <queryosity.hpp>
#include <queryosity/boost/histogram.hpp>
#include <queryosity/nlohmann/json.hpp>

using dataflow = qty::dataflow;
namespace multithread = qty::multithread;
namespace dataset = qty::dataset;
namespace column = qty::column;
namespace query = qty::query;

using json = qty::nlohmann::json;
using h1d = qty::boost::histogram::histogram<double>;
using linax = qty::boost::histogram::axis::regular;

using ull_t = unsigned long long;
auto factorial(column::observable<ull_t> n) {
  ull_t result = 1;
  auto n_factorial = n.value();
  while (n_factorial > 1)
    result *= n_factorial--;
  return result;
}
auto stirling = [](column::observable<ull_t> n) {
  return std::round(std::sqrt(2 * M_PI * n.value()) * std::pow(n.value() / std::exp(1), n.value()));
};

class Factorial : public column::definition<double(ull_t, double, ull_t)> {
public:
  Factorial(ull_t threshold = 20) : m_threshold(threshold) {}
  virtual ~Factorial() = default;
  virtual double evaluate(column::observable<ull_t> n, column::observable<double> fast,
                 column::observable<ull_t> full) const override {
    return (n.value() >= std::min<ull_t>(m_threshold, 20)) ? fast.value()
                                                    : full.value();
  }
  void adjust_threshold(ull_t threshold) { m_threshold = threshold; }

protected:
  ull_t m_threshold;
};

int main() {
  dataflow df;

  std::ifstream data_json("data.json");
  auto n = df.load(dataset::input<json>(data_json))
               .read(dataset::column<ull_t>("n"));

  auto n_f_fast = df.define(column::expression(stirling))(n);
  auto n_f_full = df.define(column::expression(factorial))(n);
  auto n_f_best =
      df.define(column::definition<Factorial>(/*20*/))(n, n_f_fast, n_f_full);

  // advanced: access per-thread instance
  ull_t n_threshold = 10;
  dataflow::node::invoke([n_threshold](Factorial *n_f) { n_f->adjust_threshold(n_threshold); },
                         n_f_best);
}
```

## Column as a series

Individual columns can be read out as arrays.

```cpp
// single column
// (sel: (varied) lazy selection)
auto x = ds.read(dataset::column<double>("x"));
auto x_series = df.get(column::series(x)).at(sel).result(); // std::vector<double>

// single column at multiple selections
// (sel_a/b/c: (varied) lazy selections)
auto [x_series_a, x_series_b, x_series_c] = df.get(column::series(x)).at(sel_a, sel_b, sel_c);

// multiple columns at a selection
auto y = ds.read(dataset::column<int>("y"));
auto z = ds.read(dataset::column<std::string>("z"));
auto [y_arr, z_arr] = sel.get(column::series(y, z));
```

```{seealso}
- [Applying selections](#applying-selections)
- [Performing queries](#performing-queries)
```