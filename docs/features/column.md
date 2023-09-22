## Reading out columns

Existing columns in a dataset can be read by specifying their type and name:
```cpp 
  auto df = ana::dataflow<table>(tabular_data);
  auto x = df.read<int>("x");
```
## Computing columns

### Simple operations

### Custom functions

### Full definitions

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

### (Advanced) Representations

For cases in which values of multiple columns in a dataset correspond to attributes of a parent entity, they can be accommodated by a `representation`.
```cpp
// example: not used in rest of walkthrough

// want to define an object corresponding to "lepton" out of four-momentum, charge, and type columns
class Lepton : public ana::column::representation<Lepton(P4,int,unsigned int)>
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
Representations provide a complementary role to definitions that can improve conceptual clarity (but not necessity) of the computation graph and (in some cases) its efficiency, demonstrated by the following aggregation-example.
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

### (Advanced) Direct instance-access of actions

No restrictions are placed on user-implementations of `column` on methods that users may want to add to the class, which can be used to "configure" them prior to the dataset processing. In such cases, access to each instance (one for each thread) can be done synchronously:
```cpp
custom_column.call_all_slots( [](CustomColumn& col){ /* call whatever methods you want to "configure" it, if implemented */ } );
```


## Computation graph

The computation graph is guaranteed to be:

- Recursion-free, as the interface forbids this by construction.
- No-copy of column values passed from one another, unless a conversion is required.
