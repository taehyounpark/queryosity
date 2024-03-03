## Read from dataset

Consider the following JSON data:
```json
[
    {"a" : 1, "b" : 2.0, "c": "three"},
    {"a" : 2, "b" : 3.0, "c": "one"},
    {"a" : 3, "b" : 1.0, "c": "two"}
]
```
??? abstract "Example implementation"
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

It can be opened by a dataflow:
```{ .cpp .annotate } 
#include <nlohmann/json.hpp>
#include "queryosity.h"

using dataflow = queryosity::dataflow;

nlohmann::json data;  // above data

dataflow df;
auto ds = df.open<queryosity::json>(data);
```

And the dataset columns can be read individually:
```cpp
auto a = ds.read<int>("a");
auto b = ds.read<double>("b");
auto c = ds.read<std::string>("c");
```
Alternatively, multiple columns can be read out in a single line:
```{ .cpp .annotate }
auto [a, b, c] = df.open<queryosity::json>(data)
                   .read<int,double,string>({"a","b","c"}); // (1)
```

1.    Note the initializer braces around the column names.

!!! info "Arbitrary column types"
    The interface is completely agnostic to the underlying column data types.
    As long the `dataset::column` of a given arbitrary type is properly implemented, it can be used.
    Even in the "worst" case, explicit template specialization can be used to cherry-pick how to read a specific data type.
    ```cpp
    template <>
    class queryosity::json::reader<CustomData> : public queryosity::column::reader<CustomData>
    { 
      virtual ColumnData const& read() const override { /* (implementation) */ }
    };
    ```
    ```cpp
    auto x = ds.read<CustomData>("x");  // success!
    ```
## Compute quantities

### Simple expressions

```cpp
auto amount = ds.read<double>("amount");
auto numbers = ds.read<std::vector<float>>("numbers");
```

```cpp
// (1)
auto hundred = df.constant(100.0);
auto first = df.constant<unsigned int>(0);

// (2)
auto percent = amount / hundred;
auto first_number = numbers[first];
```

1. Constants of arbitrary data type can be declared by providing their value. The type must be *CopyConstructable*.
2. Binary and unary actions between the underlying data types can be used. Self-assignment operators (e.g. `+=`) are not suppported.

!!! info 
    Reminder that these actions are *lazy*, and have not been performed on any particular entry or values.

### Callable expressions.

Any callable object that a `std::function` can wrap around can be used to define a column:
```cpp
auto number_of_numbers = df.define([](std::vector<float> const& vec){
    return vec.size();
    })(numbers);
```
!!! info
    1. It is best to supply large values as `const&` in order to prevent copies.
    2. The input columns to the defined column are supplied as a separate argument set. 

### Custom definitions
Complex computations can be fully specified by implementing a `definition`. 

```cpp
// shift & smear a quantity be a (relative) gaussian convolution
class GaussianConvolution : public queryosity::column::definition<double(double)>
{
  GaussianConvolution(double mu = 1.0, double sigma = 0.0) : m_mu(mu), m_sigma(sigma) {}
  virtual ~GaussianConvolution() = default;

  void setResponse(double mu, double sigma) { m_mu = mu; m_sigma = sigma; }

  virtual double evaluate(observable<double> x) {
    return x.value() * response;
  }

protected:
  double m_mu;
  double m_sigma;
};

```

This can also be used via the `dataflow::define()` method, with two ways in which individual instances can be configured at run-time:

1. Constructor arguments.
2. Custom methods on instantiated objects.

In this example, the two ways can be used to achieve the same thing

=== "Constructor"
    ```cpp
    // (1)
    auto x_conv = df.define<GaussianConvolution>(1.1, 0.1)(x);
    ```
=== "Instance direct-access"
    ```cpp
    auto x_conv = df.define<GaussianConvolution>()(x);
    std::for_each( x_conv.begin(), x_conv.end(), [](GaussianConvolution* column){ column->setResponse(1.1, 0.1); });
    ```

!!! info
    An `observable` is wrapped around each input argument of `column::definition::evaluate()` for two reasons:

    1. Its `value()` is evaluated and stored at most once per-entry, and accessed without copying.
    2. If `value()` is not called, the argument column is *not evaluated*, i.e. needless computation can be avoided.

A column definition works with arbitrary set of input and output value types. Here is another example that defines a ROOT `ROOT::Math::PtEtaPhiMVector` out of the i-th element of 4 `RVec<double>`s:

```cpp
#include <ROOT/RVec.hxx>
#include "TLorentzVector.h"

using VecD = ROOT::RVec<double>;
using P4 = ROOT::Math::PtEtaPhiMVector;

class NthP4 : public queryosity::column::definition<P4(VecD, VecD, VecD, VecD)>
{
public:
  NthP4(unsigned int index) : 
    m_index(index)
  {}
  virtual ~NthP4() = default;

  virtual P4 evaluate(queryosity::column::observable<VecD> pt, queryosity::column::observable<VecD> eta, queryosity::column::observable<VecD> phi, queryosity::column::observable<VecD> energy) const override {
  }

protected:
  const unsigned int m_index;
};

// ...

auto df = dataflow();
auto ds = dataflow.open<Tree>( {"hww.root"}, "mini" );

auto [jet_pts, jet_etas, jet_phis, jet_ms] = ds.read<VecD,VecD,VecD,VecD>({"jet_pts", "jet_etas", "jet_phis", "jet_ms"});

auto p4j1 = df.define<NthP4>(0)(jet_pts, eta_etas, jet_phis, jet_ms);
auto p4j2 = df.define<NthP4>(1)(jet_pts, eta_etas, jet_phis, jet_ms);

```

### (Advanced) Representations

For cases in which values of multiple columns in a dataset correspond to attributes of a parent entity, they can be accommodated by a `representation`.
```cpp
// example: not used in rest of walkthrough

// want to define an object corresponding to "lepton" out of four-momentum, charge, and type columns
class Lepton : public queryosity::column::representation<Lepton(P4,int,unsigned int)>
{
public:
  // helper to keep track of of properties index
  enum { p4, charge, type };
  
  Lepton() = default;
  virtual ~Lepton() = default;
  
  // can access/derive quantities from properties
  bool getP4()      { return this->value<p4>(); }
  bool getCharge()  { return this->value<charge>(); }
  bool isElectron() { return this->value<type>() == 11; }
  bool isMuon()     { return this->value<type>() == 13; }
}

// ...

auto l1 = df.define<Lepton>()(l1p4, lep_charge[0], lep_type[0]);
```
Representations provide a complementary role to definitions that can improve conceptual clarity (but not necessity) of the computation graph and (in some cases) its efficiency, demonstrated by the following query-example.
```cpp
// using a simple struct to hold properties
struct Lepton { const P4 p4; const double q; const double type; };

// straightforward to use with definition... but optimal?
auto l1 = df.define([](P4 const& p4, int q, unsigned int type){return Lepton{p4,q,type};})(l1p4,lep_charge[0],lep_type[0]);
```
Note that the following computing inefficiencies occur:

- All input column values must be evaluated in order to determine and assign the properties of the `Lepton` instance, even if only a subset of them may end up being used in the end. 
- The `Lepton` must be constructed and destructed for each entry that it is needed for.

Meanwhile, representations will not experience either of these shortcomings.