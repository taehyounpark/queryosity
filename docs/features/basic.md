An arbitrary analysis structure can be constructed as a directed graph of operations that contains a task be performed for each entry. This is called a `dataflow` object in analogical.

!!! example "A dataflow object"

    ``` mermaid 
    graph LR
    A(Column A) --Value--> X(Define X);
    B(Column B) --> X;
    Y(Define Y) --> F([Filter W]);
    B --> F;
    X --> O[(Aggregate O)];
    F -. Pass? .-> O
    ```

An operation falls into one of three categories, each associated with a set of methods:

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
| | | `define()` | Evaluate a column value per-entry |
| `selection` | A boolean/floating-point decision | `filter()` | Apply a selection | 
| | | `channel()` | Same as filter, but remember its "path" |
| `aggregation` | Perform an action and output a result | `book()` | Book the creation of a result |
| | | `fill()` | Perform aggregation with column value(s) |
| | | `at()` | Perform aggregation for entries passing the selection(s) |
