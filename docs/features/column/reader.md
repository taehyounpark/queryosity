Existing columns in a dataset can be read by specifying their type and name:
```cpp 
auto df = ana::dataflow<table>(tabular_data);
auto x = df.read<int>("x");
```
