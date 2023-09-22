A `aggregation` defines an action that is
- Performed `at()` at a selection, i.e. only if the cut has passed.
    -  Handling (or ignoring) the selection weight.
- Can be `fill()`ed with columns such that their values are also handled.

A full user-implementation must specify what (arbitrary) action is performed and its output result.

Each `fill()` and `at()` call returns a new node with those operations applied, such that any aggregation can be:
- Filled with columns any number of times, as long as their dimensionality matches that of the implementation.
- Booked at any (set of) selection(s), as long as the selections booked in each set has unique paths.

When a aggregation is booked at multiple selections such as the above, result at each selection can be accessed by its path.

If a aggregation is booked at numerous selections, it might be convenient to have a consistent way to write out the results across all selections at once. This can be done completely on the user-side or through yet another helper interface class.
