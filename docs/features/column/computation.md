Computing quantities of interest out of existing column values in an entry may be a complicated task.

```mermaid
  graph BT
  A[column] --> X[definition];
  B[column] --> X;
  B --> Y[definition];
  C[constant] --> Y;
  X --> Z[definition];
  Y --> Z;
  B --> Z;
```
Some desirable properties of the computation graph guaranteed by analogical:

- No circular loops.
- The value of a column is computed at most once per-entry, only if needed.
- Column values are not copied when used as inputs for others.
    - Only if a conversion is required, the value is copied.