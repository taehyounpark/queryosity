A **systematic variation** of an analysis constitutes a __change in a column value that affects the outcome of downstream selections and aggregations__. Processing these variations within a single computation graph at once offers the following benefits:

- Guarantee that each variation and only the variation is in effect between the nominal and varied results in the rest of the analysis logic.
- Eliminate the runtime overhead associated with repeated processing of the dataset.

## Varying a column

Any column can be varied with an alternate definition of the same type, which translates to:

- `reader` can read a different column holding the same data type.
- `constant` can be a different value.
- `equation` can be evaluated with another function of the same signature and return type.
- `definition`/`representation` can be constructed with another set of arguments (instance-access also available per-variation).

## Propagation of variations through selections and aggregations

The analysis interface works the same way, whether an action is `lazy` or `varied`, meaning:

- Any column evaluated from varied input columns containing will be varied correspondingly.
- Any selections and aggregations performed with varied columns will be varied correspondingly.

The propagation of variations across multiple actions occur "in lockstep" and "transparently", meaning:
- If two actions each have a variation of the same name, they are in effect together.
- If one action has a variation while another doesn't, then the nominal is in effect for the latter.
