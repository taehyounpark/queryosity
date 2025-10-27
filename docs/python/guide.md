# User guide

## Initialize a dataflow


```python
from qtypy import dataflow
df = dataflow(multithreaded = True)
```

## Computing columns

```python
from qtypy import dataset, column

df | {"x" : dataset.column(key="x", dtype="ROOT::RVec<float>")}  # qty::dataset::column implementation
df | {"y" : colum.expression("x.size()")                         # JIT-ed C++ expression
df | {"z" : column.definition['CustomDefinition'](*args)('x') }  # compiled qty::column::definition implementation

df | {"invalid C++ identifier!" : column.constant(1) }  # ❌
```

## Applying selections

Unlike the C++ layer, selections are *automatically* chained one after the other.

```python
from qtypy import filter, weight

df | {'all' : filter('true')}       # cut = true,       weight = 1.0
df | {'a'   : filter('a == true')}  # cut = (all && a), weight = 1.0
df | {'w'   : weight('w == true')}  # cut = (all && a), weight = (1.0 * w)
```

### Branching & merging

Diverging selections can be explicitly speicified by performing the them "at" a branching point:

```py
origin = df | {'origin' : filter('true')}
branch_ac = df @ 'origin' | {'branch_a' : filter('a == true')} | {'ac' : filter('c == true')}
branch_bd = df @ 'origin' | {'branch_b' : filter('b == true')} | {'bd' : filter('d == true')}
```

Divergent selections can be subsequently re-merged similarly:

``` py
ac_union_bd        = df @ 'origin' | {'ac_union_bd'        : filter('ac || bd')}
ac_intersection_bd = df @ 'origin' | {'ac_intersection_bd' : filter('ac && bd')}
```

## Running queries

Provided out-of-the-box are two main modes of queries covering the vast majority of ROOT users' needs for whatever their downstream analysis may be.

- `TH1`/`TTree` outputs for (Py)ROOT-based workflows.
- `numpy.ndarray` outputs for other Python-based workflows.

Or both simultaneously (since we are in Python).

### `TH1`-based histograms

There is currently support for
- `TH[1,2,3][C,I,F,D]` histograms
- `TH[1,2][F,D]Bootstrap` histograms from `BootstrapGenerator` package.

```python
from qtypy import hist

hx = df >> hist('x', dtype='float', nx=100, xmin=0.0, xmax=1.0).fill('x')
```

Remember that data processing is triggered only once the result of a query is explicitly accessed.

``` python
hx.result()  # TH1F
```

Query definitions can be "recycled" to be run at multiple selections

```python
# multiple selections
selections = ['a', 'b', 'c']

# define query without df
hx = hist('x', dtype='float', nx=100, xmin=0.0, xmax=1.0).fill('x')

# query at multiple selections
hxs = {
    sel : df @ sel >> hx for sel in selections
}

# access result at each selection
hxs['a'].result()
```

### Numpy arrays

How the Python-side `query.definition` access the underlying C++ data types can be customized.
In this case, a readout of `std::vector` as `numpy.ndarray` can be obtained as:

```python
x = df >> column.to_numpy('x', dtype='float')
x.result()  # <class 'numpy.ndarray'>, dtype('float32')
```

:::{note}
`numpy.asarray()` supports zero-copy readout of the `std::vector` only in the case of scalar elements.
For nested/arbitrary data types, a copy must be invoked when crossing the C++\-to-Python boundary.
:::