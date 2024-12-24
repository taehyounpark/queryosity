# Conceptual overview

Queryosity enables a [*dataflow*](https://en.wikipedia.org/wiki/Dataflow_programming) model of data analysis, in which a data (edges) flow between actions (nodes) in to form a directed, acyclic graph.

![dataflow](../images/dataflow.png)

An *action* belongs to one of three categories, depending on the nature of the operation.
A sub-graph of tasks within each category expresses the dependencies of each action on others, potentially of other categories, as inputs.

| Action | Description | Methods | Description | Task Graph | Inputs |
| :--- | :-- | :-- | :-- | :-- | :-- | 
| `column` | Quantity of interest | `read()` | Read a column. | Computation |  |
| | | `define()` | Compute a column. | | `column` |
| `selection` | Boolean decision | `filter()` | Apply a cut. | Cutflow | `column` |
| | Floating-point decision | `weight()` | Apply a statistical significance. | | `column` |
| `query` | Perform a query | `get()` | Define an output. | Experiment | |
| | | `fill()` | Populate with column value(s). | | `column` |
| | | `at()` | Run over selected entries. | | `selection` |
| | | `result()` | Get the result. | | |

All actions are first defined in a *lazy* fashion, meaning they are not performed unless its result is accessed by the user.
The eagerness of actions in each entry is as follows:

1. A query is performed only if its associated selection passes the cut.
2. A selection is applied only if all prior cuts in the cutflow have passed.
3. A column is evaluated only if it is needed to determine any of the above.

## Columns

Column
: An action that holds a value of some data type `T` to be updated for each entry.

Independent column
: A column whose value does not depend on others

Dependent column
: A column whose value is evaluated out of those from other columns as inputs.

***

The tower of dependent columns can be constructed to form the computation graph:

:::{card}
:text-align: center
![computation](../images/computation.png)
+++
Example computation graph.
:::

Only the minimum number of computations needed are performed for each entry:
- A column value is computed *once* for an entry (if needed), then cached and never re-computed.
- A column value is not copied when used as an input for dependent columns (unless a conversion is needed).

## Selections

Selection
: A scalar-valued column corresponding to a "decision" on an entry:
  - A boolean `cut` to determine if a query should be performed for the entry.
    - A series of two or more cuts becomes their intersection, `and`
  - A floating-point `weight` to assign a statistical significance to the entry.
    - A series of two or more weights becomes to their product, `*`.

***

A cutflow can contain the following types of connections between selections:

- Applying a selection from an existing node, which determines the order in which they are compounded.
- Branching selections by applying more than one selection from a common node.
- Merging two selections, e.g. taking the union/intersection of two cuts.

:::{card}
:text-align: center
![cutflow](../images/cutflow.png)
+++
Example cutflow.
:::

Selections constitute a specific type of columns, so they are subject to the lazy-evaluation and value-caching behaviour of the computation graph.
Addditionally, the cutflow imposes the following rules:
- The cut at a selection is evaluated only if all previous cuts have passed.
- The weight at a selection is evaluated only if its cut has passed.

## Queries

Query
: An action that outputs result of some data type `T` after traversing the dataset.
  - It must be associated with a selection whose cut determines which entries to count.
    - (Optional) The result is populated with the weight taken into account.
  - How the query counts an entry is a user-implemented arbitrary action.
    - (Optional) The result is populated based on values of inputs columns.

***

:::{card}
:text-align: center
```{image} ../images/query_1.png
+++
Making, filling, and booking a query.
:::
