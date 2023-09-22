The centerpiece of this library is a directed graph of computational tasks performed for each entry of a tabular dataset, called a `dataflow` object.
A node can be one of the following three things:

| Operation     | Description                          |
| :------------ | :------------------------------------ |
| `column`      | Access the value of a speciic data type  |
| `selection`   | Return a boolean (`cut`) or a floating-point (`weight`) decision |
| `aggregation` | Perform out arbitrary action and output a result |
