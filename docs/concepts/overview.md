An arbitrary analysis consists of a directed, acyclic graph of tasks performed for each entry. This is called a `dataflow` object.

![dataflow](../assets/dataflow.png)

Each `node` comprises an action to be performed per-entry, and falls into one of three categories associated with a set of applicable methods:

<style>
table th:first-of-type {
    width: 15%;
}
table th:nth-of-type(2) {
    width: 20%;
}
table th:nth-of-type(3) {
    width: 15%;
}
table th:nth-of-type(4) {
    width: 40%;
}
</style>

| Action | Description | Methods | Description |
| :------------ | :------------------------------------ | :------------ | :------------------------------------ |
| `column` | Access or evaluate a quantity | `read()` | Read a column. |
| | | `define()` | Evaluate a column. |
| `selection` | A boolean/floating-point decision | `filter()` | Apply a cut. | 
| | | `weight()` | Apply a statistical significance. |
| `query` | Perform a query | `make()` | Make a query plan. |
| | | `fill()` | Fill query with column value(s) of the entry. |
| | | `book()` | Perform query over selected entries. |
