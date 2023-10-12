To perform a sensitivity analysis means to determine how variations in the input of a system affect its output.

## Systematic variation

A **systematic variation** constitutes a __change in a column value that affects the outcome of selections and aggregations__.
Processing these them within a single dataflow object offers the following benefits over applying them independently:

- Guarantee by construction that only one variation at a time is in effect.
- Eliminate runtime overhead associated with repeated dataset traversal.

## Available variations

Any column can be varied with an alternate constructor of the same type, which translates to:

| Column | Variation |
| :--- | :--- |
| `dataset::column` | Column name |
| `constant` | Value |
| `equation` | Callable expression |
| `definition`/`representation` | Constructor arguments of the custom class and direct-access |

## Propagation of variations

Nominal and varied operations are automatically carried forward during an analysis, meaning:

- Any dependent columns and selections evaluated from varied input columns will be varied.
- Any aggregations performed using varied columns and selections will be varied.

The propagation proceeds "in lockstep" and "transparently", meaning:

- If two actions each have a variation of the same name, they are in effect together.
- If one action has a variation while another doesn't, then the nominal is in effect for the latter.