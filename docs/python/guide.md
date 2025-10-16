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
