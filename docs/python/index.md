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

The Python interface aims to enable a more idiomatic use of Python’s dynamic typing and flexibility, while maintaining a clear separation between C++ expressions—which must always be provided as str—and native Python operations.

There are two equivalent ways to express a dataflow:

1. Methods
2. Operators

These two approaches are fully interchangeable. Users are free to use either—or mix both—depending on their preferred syntax.

::::{tab-set}
:::{tab-item} Methods

```{code-block} python
from qtypy import dataflow, dataset, column, filter

df = dataflow(multithread=True, n_threads = 8)
df.input(dataset.tree(file_paths=["data.root"], tree_name="events"))

df.compute(
    {
        "x": dataset.column("x", dtype="float"),
        "sqrtx": column.expression("sqrt(x)"),
    }
)

df_sqrtx_valid = df.filter({"sqrtx_valid": "x >= 0"})
assert(df_sqrtx_valid == df.at("sqrtx_valid"))

q_sqrtx = df_sqrtx_valid.get(column.to_numpy("sqrtx")).result()
sqrt_x = q_sqrtx.result()
```
:::

:::{tab-item} Operators
```{code-block} python
from qtypy import dataflow, dataset, column, filter

df = dataflow(multithread=True, n_threads = 8)
df << dataset.tree(file_paths=["data.root"], tree_name="events")

df["x"] = dataset.column("x", dtype="float")
df["sqrtx"] = "sqrt(x)"

df_sqrtx_valid = df & {"sqrtx_valid" : "x >= 0"}
assert(df_sqrtx_valid == (df @ "sqrtx_valid"))

q_sqrtx = df_sqrtx_valid >> column.to_numpy("sqrtx")
sqrtx = q.result()
```
:::