# Basic bits

```python
from qtypy import dataflow
df = dataflow(multithreaded = True)
```

## Computing columns

A column can be computed in 4 ways:

- `dataset.column` whose value is read in from an existing dataset.
- `column.constant` whose value does not change throughout per-event.
- `column.expression`, or simply a `str`, that is jitted using Cling.
- `column.definition`, a user-compiled C++ computation unit (see API reference for more details).

```python
from qtypy import dataset, column

df["x"] = dataset.column(key="x", dtype="ROOT::RVec<float>")
df["y"] = "x.size()"

df["invalid C++ identifier"] = column.constant(1)  # ❌
```

## Applying selections

By default, successive selection operations are always compounded one after another:

```python
(
df
.filter({"all" : "true"})       # cut = true,       weight = 1.0
.filter({"a"   : "a == true"})  # cut = (all && a), weight = 1.0
.weight({"w"   : "w == true"})  # cut = (all && a), weight = (1.0 * w)
)
```

### Branching & merging

Diverging selections can be applied by specifying they be applied "at" a branching point:

```py
a = df & {"a" : "a == true"}
a_and_b = df @ "a" & {"a_and_b" : "b == true"}
a_and_b = df @ "a" & {"a_and_b" : "c == true"}
```

Divergent selections can be subsequently re-merged similarly:

``` py
intersection = (df @ "a") & {"intersection" : "a_and_b && a_and_c"}
union        = (df @ "a") & {"union"  : "a_and_b || a_and_c"}
```

## Histogram outputs

```python
from qtypy import hist

# from whatever the last applied selection was
hx = df >> hist("x", dtype="float", nx=100, xmin=0.0, xmax=1.0).fill("x")
```

```python
df @ "base" >> hist("x", dtype="float", nx=100, xmin=0.0, xmax=1.0).fill("x")
```

Remember that data processing is triggered only once the result of a query is explicitly accessed:

``` python
hx.result()  # TH1F
```

Multiple queries can be performed at once (of course). Query definitions can be "recycled" to be run at multiple selections, for example:

```python
# multiple selections
selections = ["a", "b", "c"]

# define query without df
hx = hist("x", dtype="float", nx=100, xmin=0.0, xmax=1.0).fill("x")

# query at multiple selections
hxs = {
    sel : df @ sel >> hx for sel in selections
}
```

:::{important}
:name: 
Accessing the result of a query triggers the event-loop (if needed). In order to prevent running it multiple times, make sure that you have specified all queries needed at a time before accessing any of their results!
:::

## Numpy array outputs

```python
x = df >> column.to_numpy("x", dtype="float")
x.result()  # <class "numpy.ndarray">, dtype("float32")
```

:::{note}
`numpy.asarray()` supports zero-copy readout of the `std::vector` only in the case of scalar elements.
For nested/arbitrary data types, a copy must be invoked when crossing the C++\-to-Python boundary.
:::