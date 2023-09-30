## Existing columns in input data
Existing columns in a dataset can be read by specifying their type and name:
```cpp 
auto x = df.read<int>("x");
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

!!! info "Arbitrary column types"
    The interface is agnostic (oblivious, to be exact) to the column data types being read.
    As long the `dataset::column` of a given arbitrary type is properly implemented, it can be used.
    ```cpp
    // explicit template specialization for DataType
    template <>
    class Tree::Branch<DataType> : public ana::dataset::column<DataType> { /* ... */ };
    ```
    ```cpp
    auto y = df.read<DataType>("y");  // <- success!
    ```
## Simple expressions
Binary and unary operators for columns with applicable underlying data types can be used:
```cpp
auto amount = df.read<double>("amount");
auto percent = amount / df.constant(100.0);
```
```cpp
auto numbers = df.read<std::vector<float>>("numbers");
auto first_number = numbers[df.constant(0)];
```
!!! note 
    This will look deceivingly obvious; keep in mind that these operations are *lazy*, and have not been performed on any particular entry or values; they have simply been defined.

Self-assigment operators (`+=`,`-=`,etc.) are not available.
## Callables (functions, lambdas, etc.)

Any callable object that a `std::function` can wrap around can be used to define a column:
```cpp
auto numbers_count = df.define([](std::vector<float> const& vec){
    return vec.size();
    })(numbers);
```

## Custom definitions
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
