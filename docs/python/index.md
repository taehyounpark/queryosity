# Python

```{toctree}
:hidden: false
:maxdepth: 1

install
guide
```


```{toctree}
:hidden: false
:caption: API reference
:maxdepth: 1

api/dataflow
api/dataset
api/column
api/selection
api/query
```

```{code-block}
from qtypy import dataflow, dataset
from qtypy.column import constant, expression, definition
from qtypy.selection import filter, weight
from qtypy.query import hist

df = dataflow(n_threads = 8)

df << {'events' : dataset.tree(file_paths=['data.root'], tree_name='events')} 
df | {'x': dataset.column('x', value_type='float')}
   | {'sqrtx': expression('sqrt(x)')}

df @ {'xpos': filter('x >= 0')}

q = (
    df >> {'h_sqrtx': hist('h', nx=100, xmin=0.0, xmax=1.0).fill('sqrtx') @ ['xpos']}
).get()

print(q['h_sqrtx'])
)

print(df.results['x_valid']['h_y'])
```