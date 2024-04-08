# Yield at a selection

```cpp
// single selection
auto all = df.filter(column::constant(true));
auto yield_tot = def.get(selection::yield(all));
unsigned long long yield_tot_entries =
    yield_tot.entries;                    // number of entries passed
double yield_tot_value = yield_tot.value; // sum(weights)
double yield_tot_error = yield_tot.error; // sqrt(sum(weights squared))

// multiple selections 
// (sel_a/b/c: (varied) lazy selections)
auto [yield_a, yield_b, yield_c] =
    df.get(selection::yield(sel_a, sel_b, sel_c));
```