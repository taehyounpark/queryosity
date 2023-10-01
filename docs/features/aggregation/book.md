A `aggregation` defines an action that:

- Executes at a particular selection, i.e. only if the cut has passed.
    -  (Optional) Takes into account the weight.
- (Optional) Populate the aggregation output with a given set of column values.
- Outputs a result after the full dataset has been traversed.

The action and output is implemented by the analyzer.