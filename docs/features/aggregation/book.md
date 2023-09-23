A `aggregation` defines an action that is:

- Booked "at" at a selection, i.e. only if the cut has passed.
    -  Handles (or ignores) the selection weight.
- (optional) Can be "fill"ed with columns whose values are used in the action.
- Outputs a result after the full dataset has been traversed.

The action and output is implemented by the analyzer.

Each `fill()` and `at()` call returns a new node with those operations applied, such that any aggregation can be:

- Filled with columns any number of times, as long as their dimensionality matches that of the implementation.
- Booked at any (set of) selection(s), as long as the selections booked in each set has unique paths.

When an aggregation is booked at multiple selections such as the above, result at each selection can be accessed by its path.