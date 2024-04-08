# Computing quantities

New columns can be computed out of existing ones by calling `queryosity::dataflow::define()` with the appropriate argument, or operators between the underlying value types.

:::{admonition} Template
```{code} cpp
auto cnst = df.define(column::constant<DTYPE>(VALUE));
auto eqn  = df.define(column::expression(EXPRESSION))(COLUMNS...);
auto defn = df.define(column::definition<DEFINITION>(ARGUMENTS...))(COLUMNS...);
```
:::

:::{admonition} Requirements on column value type
:class: important
A computed column **MUST** output a value of a type `T` that is:
- {{DefaultConstructible}}.
- {{CopyAssignable}} or {{MoveAssignable}}.
:::

## Basic operations

```cpp
// constants columns do not change per-entry
auto zero = df.define(column::constant(0));
auto one = df.define(column::constant(1));
auto two = df.define(column::constant(2));

// binary/unary operators
auto three = one + two;
auto v_0 = v[zero];
// reminder: actions are *lazy*, i.e. no undefined behaviour (yet)

// can be re-assigned as long as value type remains unchanged
two = three - one;
```

## Custom expressions

Any C++ Callable object (function, functor, lambda, etc.) can be used to evaluate a column. Input columns to be used as arguments to the function should be provided separately.

```cpp
auto s_length = df.define(
    column::expression([](const std::string &txt) { return txt.length(); }))(s);
```
:::{tip}
Pass large values by `const &` to avoid expensive copies.
:::

## Custom definitions

A column can also be computed through a custom column definition, which enables full control over its

- Customization: user-defined constructor arguments and member variables/functions.
- Optimization: the computation of each input column is deferred until its value is invoked.

:::{admonition} Thread-safety requirements
:class: caution
- Each instance of a custom definition remain in a thread-safe state.
- Its constructor arguments must be {{CopyConstructible}}.
:::

Consider the following example of computing the factorial of a number, $ n! = 1 \times 2 \times 3 \times ... \times n $:

1. if $ n! $ is too small and/or can fit inside `unsigned long long`, use full calculation.
2. if $ n $ is large enough, use Stirling's approximation.

```cpp
#include <fstream>
#include <sstream>
#include <vector>

#include "queryosity.h"
#include "queryosity/hist.h"
#include "queryosity/json.h"

using dataflow = qty::dataflow;
namespace multithread = qty::multithread;
namespace dataset = qty::dataset;
namespace column = qty::column;
namespace query = qty::query;

using json = qty::json;
using h1d = qty::hist::hist<double>;
using linax = qty::hist::axis::regular;

using ull_t = unsigned long long;
auto factorial(ull_t n) {
  ull_t result = 1;
  while (n > 1)
    result *= n--;
  return result;
}

auto stirling = [](ull_t n) {
  return std::round(std::sqrt(2 * M_PI * n) * std::pow(n / std::exp(1), n));
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

  ull_t n_threshold = 10;
  auto n_f_slow = df.define(
      column::expression([n_threshold](ull_t n, double fast, ull_t slow) -> double {
        return n >= std::min<ull_t>(n_threshold,20) ? fast : slow;
      }))(n, n_f_fast, n_f_full);
  // time elapsed = t(n) + t(fast) + t(slow)
  // :(

  auto n_f_best =
      df.define(column::definition<Factorial>(/*20*/))(n, n_f_fast, n_f_full);
  // time elapsed = t(n) + { t(n_fast) if n >= 10, t(n_slow) if n < 10 }
  // :)

  // advanced: access per-thread instance
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