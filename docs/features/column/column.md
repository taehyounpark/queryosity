## Existing columns in input dataset

Consider the following JSON data and a suitable `dataset` implementation for it:
```json
[
    {"a" : 1, "b" : 2.0, "c": "three"},
    {"a" : 2, "b" : 3.0, "c": "one"},
    {"a" : 3, "b" : 1.0, "c": "two"}
]
```
??? abstract "Implementation source code"
    ```cpp
    template <typename T>
    class Tree::Branch : public ana::dataset::column<T>
    {

    public:
        Branch(const std::string& branchName, TTreeReader& treeReader) :
            m_branchName(branchName),
            m_treeReader(&treeReader)
        {}
        ~Branch() = default;

        virtual void initialize(const ana::dataset::range&) override
        {
            m_treeReaderValue = std::make_unique<TTreeReaderValue<T>>(*m_treeReader,this->m_branchName.c_str());
        }

        virtual T const& read(const ana::dataset::range&, unsigned long long) const override
        {
            return **m_treeReaderValue;
        }

    protected:
    std::string m_branchName;
    TTreeReader* m_treeReader;
    std::unique_ptr<TTreeReaderValue<T>> m_treeReaderValue;  

    };
    ```

Then a dataflow object can open any such dataset by:
```{ .cpp .annotate } 
#include <nlohmann/json.hpp>

nlohmann::json data;  // above data
auto ds = df.open<ana::json>(data);
```

And the dataset columns can be read individually by:
```cpp
auto a = ds.read<int>("a");
auto b = ds.read<double>("b");
auto c = ds.read<std::string>("c");
```
Alternatively, they can all be done in a single line as:
```{ .cpp .annotate }
auto [a, b, c] = df.open<ana::json>(data)\
                   .read<int,double,string>({"a","b","c"}); // (1)
```

1.    Only difference: the initializer braces around the column names.

!!! info "Arbitrary column types"
    The interface is agnostic (oblivious, to be exact) to the column data types being read.
    As long the `dataset::column` of a given arbitrary type is properly implemented, it can be used.
    For example, an explicit template specialization can be used to cherry-pick how to read specific column data types.
    ```cpp
    template <>
    class ana::json::column<CustomData> : public ana::dataset::column<CustomData>
    { 
        /* (valid implementation here) */ 
    };
    ```
    ```cpp
    auto x = ds.read<CustomData>("x");  // success!
    ```
## Computing columns

Any other column is defined through the dataflow object.

### Constant

```cpp
auto amount = ds.read<double>("amount");  // read from dataset
auto hundred = df.constant(100.0);  // defined from dataflow
```

### Simple expressions
Binary and unary operators for columns with applicable underlying data types can be used:
```cpp
auto percent = amount / df.constant(100.0);
```
```cpp
auto numbers = ds.read<std::vector<float>>("numbers");
auto first = df.constant(0);
auto first_number = numbers[first];
```
!!! note 
    This might look deceivingly obvious; keep in mind that these operations are *lazy*, and have not been performed on any particular entry or values.

Self-assigment operators (`+=`,`-=`,etc.) are not available.

### Callables (functions, lambdas, etc.)

Any callable object that a `std::function` can wrap around can be used to define a column:
```cpp
auto numbers_count = df.define([](std::vector<float> const& vec){
    return vec.size();
    })(numbers);
```

### Custom definitions
Complex computations can be fully specified by implementing a `definition`. 

```cpp

class NthP4 : public ana::column::definition<P4(VecD, VecD, VecD, VecD)>
{
public:
  NthP4(unsigned int index) : 
    m_index(index)
  {}
  virtual ~NthP4() = default;

  virtual P4 evaluate(ana::observable<VecD> pt, ana::observable<VecD> eta, ana::observable<VecD> phi, ana::observable<VecD> energy) const override {
  }

protected:
  const unsigned int m_index;
};
```
??? note "(Advanced) Direct-access of columns"

    In principle, custom definitions can contain any additional methods that the implementer adds.
    The existence of those methods is entirely irrelevant as far as the interface is concerned.
    On the other hand, they can provide a useful way to "configure" the columns further than just with their constructor arguments.
    ```cpp
    custom_column.call_all_slots( [](CustomColumn& col){ /* call whatever methods you want to "configure" it, if implemented */ } );
    ```
