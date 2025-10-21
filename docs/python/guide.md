# User guide

## Initialize a dataflow


```python
from qtypy import dataflow, dataset
from qtypy.column import constant, expression, definition
from qtypy.selection import at, cut, weight
from qtypy.query import hist
```

```python
df = dataflow(multithreaded = True, n_threas = 8)
```

## Reading and defining columns

```python
df.read(
    {
    "x" : dataset.column(key="x", value_type="float"),
    }
)

y = column.expression("x+1")
df.define(
    {
    "y" : y
    }
)
```

## Applying selections

```python

```