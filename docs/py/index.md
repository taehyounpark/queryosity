# Welcome to qtypy's documentation

:::{card} 
qtypy is a Python latyer for data analyses written in [queryosity](https://queryosity.readthedocs.io).
:::

```{toctree}
:hidden: true
:maxdepth: 1

guide/index
api/index
```

```{code-block}
from qtypy import dataflow, dataset
from qtypy.column import constant, expression, definition
from qtypy.selection import at, cut, weight
from qtypy.query import hist

df = (
    dataflow(n_threads = 8) 
    .load(dataset.tree(file_paths=['data.root'], tree_name='events')) 
    .read({'x': dataset.column('x', value_type='ROOT::RVec<float>')}) 
    .define({'y': expression('x[0]*x[1]')}) 
    .apply({'x_valid': cut('x.size() >= 2')}) 
    .get({'h_y': hist('y', nx=100, xmin=0.0, xmax=1.0).fill('y').at('x_valid')})
)

print(df.results['x_valid']['h_y'])
```