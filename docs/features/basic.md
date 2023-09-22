Dataset transformations consist of a set of operations that fall into one of these categories:

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

| Operation | Description | Methods | Description |
| :------------ | :------------------------------------ | :------------ | :------------------------------------ |
| `column` | Access or evaluate a quantity | `read()` | Read the value of a column |
| | | `constant()` | Set a constant value of a column |
| | | `define()` | Evaluate a column value per-entry |
| `selection` | A boolean/floating-point decision | `filter()` | Apply a selection | 
| | | `channel()` | Apply a selection, remember its "path" |
| `aggregation` | Perform an action and output a result | `book()` | Book the creation of a result |
| | | `fill()` | Perform aggregation with column values |
| | | `at()` | Perform aggregation for entries passing the selection(s) |

It follows that any arbitrary analysis structure can be constructed by creating and connecting each task as nodes of a directed computation graph to be performed for each entry of a tabular dataset, called a `dataflow` object.