:heart: [nlohmann::json](https://json.nlohmann.me/)

!!! note
    - For simplicity, the following examples exclusively use standard C++ types as column data.
    - Reminder that any type or class (as long as it satisfies the requirements below) can be used!

## Loading the dataset

First, a dataflow must load an input dataset. The signature is as follows:

```cpp
auto ds = df.load( dataset::input</*(1)!*/>(/*(2)!*/) );
```

1. `dataset::reader` implementation
2. Constructor arguments for implementation.

For example, consider the following JSON data:

```{.json .no-copy title="data.json"}
[
  {"x": 1, "y": [1.0],     "z": "a"},
  {"x": 2, "y": [],        "z": "b"},
  {"x": 3, "y": [2.0,0.5], "z": "c"}
]
```
```cpp
std::ifstream data_file("data.json");

using json = qty::json;
auto ds = df.load( dataset::input<json>(data_file) );
```
??? abstract "qty::json implementation"
    ```cpp
    #include <nlohmann/json.hpp>
    #include "queryosity/queryosity.h"

    namespace queryosity {

    class json : public queryosity::dataset::reader<json> {

    public:
    template <typename T> class column;

    public:
    json(const nlohmann::json &data);
    ~json() = default;

    queryosity::dataset::partition parallelize();

    template <typename T>
    std::unique_ptr<column<T>> read(unsigned int,
                                const std::string &name) const;

    protected:
    nlohmann::json m_data;
    };

    template <typename T> class json::column : public queryosity::column::reader<T> {

    public:
    column(const nlohmann::json &data, const std::string &name);
    ~column() = default;

    virtual const T &read(unsigned int,
                        unsigned long long entry) const override;

    protected:
    mutable T m_value;
    const nlohmann::json &m_data;
    const std::string m_name;
    };

    } // namespace queryosity

    queryosity::json::json(nlohmann::json const &data) : m_data(data) {}

    template <typename T>
    queryosity::json::reader<T>::column(nlohmann::json const &data,
                            const std::string &name)
    : m_data(data), m_name(name) {}

    queryosity::dataset::partition queryosity::json::parallelize() {
    queryosity::dataset::partition parts;
    auto nentries = m_data.size();
    for (unsigned int i = 0; i < nentries; ++i) {
    parts.emplace_back(i, i, i + 1);
    }
    return parts;
    }

    template <typename Val>
    std::unique_ptr<queryosity::json::reader<Val>>
    queryosity::json::read(const queryosity::dataset::range &, const std::string &name) const {
    return std::make_unique<column<Val>>(this->m_data, name);
    }

    template <typename T>
    const T &queryosity::json::reader<T>::read(const queryosity::dataset::range &,
                                    unsigned long long entry) const {
    m_value = this->m_data[entry][m_name].template get<T>();
    return m_value;
    }
    ```

## Reading columns

The loaded dataset can then read columns contained within it.

```cpp
auto x = ds.read( dataset::column</*(1)!*/>(/*(2)!*/) );
```

1. Underlying column data type.
2. Column name.

```{ .cpp .no-copy }
auto x = ds.read( dataset::column<int>("x") );
auto y = ds.read( dataset::column<std::vector<float>>("y") );
auto z = ds.read( dataset::column<std::string>("z") );
```
??? abstract "qty::json::entry implementation"

Reading columns from an input dataset can be done in one line:

=== "More concise"

    ```{ .cpp .no-copy }
    auto [x, y, z] = df.read(
      dataset::input<json>(data_file),
      dataset::column<int>("x"),
      dataset::column<std::vector<float>>("y"),
      dataset::column<std::string>("z")
    );
    ```

=== "Even more concise"

    ```{ .cpp .no-copy }
    auto [x, y, z] = df.read(
      dataset::input<json>(data_file),
      dataset::columns<int,std::vector<float>,std::string>("x","y","z")
    );
    ```

## Computing columns

The underlying data type `T` must be:

- *DefaultConstructible*, and
- *CopyConstructble* and *CopyAssignable*, or
- *MoveConstructible* and *MoveAssignable*.

### Constant

A constant is the simplest type of column, as its value does not change on a per-entry basis.

```cpp
auto c = df.define( column::constant</*(1)!*/>(/*(2)!*/) );
```

1. Data type (optional, automatically deduced).
2. Data value.

```{.cpp .no-copy }
auto half = df.define( column::constant(0.5) );
auto zero = df.define( column::constant<int>(0) );
```

### Operator

Binary and unary operators on underlying data types can be used.

```{.cpp .no-copy }
auto one = half + half;
auto y0 = y[zero];
```

- Self-assignment operators (e.g. `+=`) are not supported.

!!! info
    - No undefined behaviour is invoked from `y0`, even if `y` might be empty in some entries.
    - Remember: all actions here are lazy, and nothing is actually being computed (yet)!

### Expression

This is the simplest way to evaluate quantities out of existing columns involving non-trivial fields and methods.

```cpp
auto e = df.define( column::expression(/*(1)!*/), /*(2)!*/ );
```

1. A function, functor, or any other callable object from which a `std::function` can be constructed & called concurrently.
2. Input columns whose values are used as function arguments.

```{.cpp .no-copy }
                           //(1)!
auto txt_length = [](const std::string& txt){return txt.length();};
auto len_z = df.define( column::expression(txt_length), z );
```

1. It is best to pass large data types as `const &` to prevent copies.

### Definition

This is the most verbose, but customizable and performant way to define columns.

```cpp
auto d = df.define( column::definition</*(1)!*/>(/*(2)!*/), /*(3)!*/ );
```

1. `column::definition<Ret(Args...)>` implementation.
2. Constructor arguments for implementation.
3. Input columns used as arguments.

```{.cpp .no-copy }
class sum : public column::definition<double(double,double)>
{                                     //(5)!
public:
  //(1)!
  sum() = default;
  virtual ~sum() = default;
  virtual double evaluate(
    column::observable<double> a,/*(2)!*/
    column::observable<double> b) const override {
    return a.value() + b.value();
  }
  //(3)!
protected:
  //(4)!
};
```

1. Custom constructor arguments.
2. Each input column is passed as a `column::observable`, which preserves its laziness even within the entry-loop, i.e. it is not computed unless `value()` is called. (see below)
3. Custom member functions.
4. Custom member variables.
5. The definition is optimized (no-copy) for `double`, but will still work for all other compatible types (e.g. `float`), which are converted first.

```{.cpp .no-copy }
auto c = df.define( column::definition<sum>(), a, b );
```

At first glance, this just looks like a lot of boilerplate code for just doing `auto c = a+b`.
But there are ample opportunities (via inheritance) to customize the behaviour for more complicated definitions.

!!! tip "Power of observables"

    Passing an input column as a `column::observable`, which defers its computation until `value()` is invoked, can potentially result in significant performance gains.
    In general, input columns must be computed prior to evaluating the defined column, always incurs their associated computational costs.
    With observables, analyzers can pick and choose which ones need to be computed versus which can be ignored to save on unnecessary compute time.

    Consider a scenario in which there are two methods to compute a quantity:

    1. An approximation, computationally-light, and
    2. The full method, computationally-heavy.

    Suppose there is also an per-entry decision that indicates whether or not the accuracy of the approximation is sufficient for it to be used in lieu of the full method.

    As an expression:
    ```cpp
    auto x = df.define(
                            /*(1)!*/
      column::expression([](bool good, double fast, double full){return good ? fast : full;}),
      x_fast_is_accurate, x_fast, x_full
      );
    ```

    1. All arguments must be computed before entering the function.

    As a definition:
    ```cpp
    class OptimalQuantity : public column::definition<double(bool,double,double)>
    {
    public:
      OptimalQuantity() = default;
      virtual ~OptimalQuantity() = default;
      virtual double evaluate(
        column::observable<bool> good  /*(1)!*/
        column::observable<double> fast,
        column::observable<double> full) const override {
        return good.value() ? fast.value() : full.value();
               /*(2)!*/            /*(3)!*/            /*(4)!*/
      }
    };
    ```

    1. Arguments are not column values: they are not computed yet...
    2. Always computed.
    3. Small performance gain if `good.value() == false`.
    4. Big performance gain if `good.value() == true`!

!!! warning "Thread-safety requirements"

    Usage of the custom constructor arguments and member functions/variables must preserve each column instasnce in a thread-safe state.