
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
### (Advanced) Direct instance-access of actions

No restrictions are placed on user-implementations of `column` on methods that users may want to add to the class, which can be used to "configure" them prior to the dataset processing. In such cases, access to each instance (one for each thread) can be done synchronously:
```cpp
custom_column.call_all_slots( [](CustomColumn& col){ /* call whatever methods you want to "configure" it, if implemented */ } );
```
