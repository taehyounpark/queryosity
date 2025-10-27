# (Python)

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

The Python interface is intentionally not a one-to-one mirror of the C++ backend. This design enables more idiomatic use of Python’s dynamic typing and flexibility, while maintaining a clear separation between jitted C++ expressions---which are always provided as `str`s---and native Python operations.


::::{tab-set}
:::{tab-item} Method chaining

```{code-block} python
from qtypy import dataflow, dataset, column, filter

df = dataflow(n_threads = 8)
df.input(dataset.tree(file_paths=['data.root'], tree_name='events'))

df.compute(
    {'x': dataset.column('x', dtype='float')},
    {'sqrtx': column.expression('sqrt(x)')},
    {'x_geq_0': filter('x >= 0')}
)

sqrtx = df.output(column.to_numpy('sqrtx')).result()
```
:::

:::{tab-item} Pipe operator
```{code-block} python
from qtypy import dataflow, dataset, column, filter

df = dataflow(n_threads = 8)
df << dataset.tree(file_paths=['data.root'], tree_name='events')

(
df
| {'x': dataset.column('x', dtype='float')}
| {'sqrtx': column.expression('sqrt(x)')}
| {'x_geq_0': filter('x >= 0')}
)

q = df >> column.to_numpy('sqrtx')
sqrtx = q.result()
```
:::