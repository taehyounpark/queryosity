# Conceptual overview

## Dataflow

A `dataflow` consists of a directed, acyclic graph of tasks performed for each entry.

![dataflow](../images/dataflow.png)

An action is a node belonging to one of three task sub-graphs, each of which are associated with a set of applicable methods.
Actions of each task graph can receive ones of the previous graphs as inputs:

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

## Lazy actions

All actions are *lazy*, meaning they are not executed them unless required.
Accessing the result of a query turns it and all other actions *eager*, triggering the dataset traversal.
The eagerness of actions in each entry is as follows:

1. A query is performed only if its associated selection passes the cut.
2. A selection is evaluated only if all prior cuts in the cutflow have passed.
3. A column is evaluated only if it is needed to determine any of the above.

## Columns

A `column` holds a value of some data type `T` to be updated for each entry.
Columns that are read-in from a dataset or user-defined constants are *independent*, i.e. their values do not depend on others, whereas columns evaluated out of existing ones as inputs are *dependent*.
The tower of dependent columns evaluated out of more independent ones forms the computation graph:

![computation](../images/computation.png)

Only the minimum number of computations needed are performed for each entry:
- If and when a column value is computed for an entry, it is cached and never re-computed.
- A column value is not copied when used as an input for dependent columns.
    - It *is* copied if a conversion is required.

## Selections

A `selection` represents a scalar-valued decision made on an entry:

- A boolean `cut` to determine if a query should be performed for a given entry.
    - A series of two or more cuts becomes their intersection, `and`
- A floating-point `weight` to assign a statistical significance to the entry.
    - A series of two or more weights becomes to their product, `*`.

A cutflow can have from the following types connections between selections:

![cutflow](../images/cutflow.png)

- Applying a selection from an existing node, which determines the order in which they are compounded.
- Branching selections by applying more than one selection from a common node.
- Merging two selections, e.g. taking the union/intersection of two cuts.

Selections constitute a specific type of columns; as such, they are subject to the value-caching and evaluation behaviour of the computation graph.
Addditionally, the cutflow imposes the following rules on them:
- The cut at a selection is evaluated only if all previous cuts have passed.
- The weight at a selection is evaluated only if its cut has passed.

## Queries

A `query` specifies an output result obtained from counting entries of the dataset.
For multithreaded runs, the user must also define how outputs from individual threads should be merged together to yield a result representative of the full dataset.

- It must be associated with a selection whose cut determines which entries to count.
    - (Optional) The result is populated with the weight taken into account.
- How an entry populates the query depends on its implementation.
    - (Optional) The result is populated based on values of inputs columns.

Two common workflows exist in associating queries with selections:

@image html query_1.png "Running a single query at multiple selections."

@image html query_2.png "Running multiple queries at a selection."

@section conceptual-variations Systematic variations

A sensitivity analysis means to study how changes in the system's inputs affect the output. 
In the context of dataset queries, a **systematic variation** constitutes a __change in a column value that affects the outcome of selections and queries__.

Encapsulating the nominal and variations of a column creates a `varied` node in which each variation is mapped by the name of its associated systematic variation.
A varied node can be treated functionally identical to a non-varied one, with all nominal+variations being propagated through the relevant task graphs implicitly:

- Any column definitions and selections evaluated out of varied input columns will be varied.
- Any queries performed filled with varied input columns and/or at varied selections will be varied.

The propagation proceeds in the following fashion:

- **Lockstep.** If two actions each have a variation of the same name, they are in effect together.
- **Transparent.** If only one action has a given variation, then the nominal is in effect for the other.

All variations are processed at once in a single dataset traversal; in other words, they do not incur any additional runtime overhead other than what is needed to perform the actions themselves.

@image html variation.png "Propagation of systematic variations."

@see @ref guide