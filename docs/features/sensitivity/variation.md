To perform a sensitivity analysis means to determine how variations in the input of a system affect its output.

A **systematic variation** constitutes a __change in a column value that affects the outcome of selections and queries__.
There is built-in support to handle these within a single dataflow graph, which offer the following benefits:

- Guarantee that only one variation is in effect at a time versus a nominal, by construction.
- Eliminate runtime overhead associated with repeated dataset traversals.

Any column can be varied within the same type, which translates to:

| Column | Variation |
| :--- | :--- |
| `reader` | Read a different column of the same data type |
| `constant` | Set a different value of the same data type |
| `equation` | Use a different expression (with compatible function arguments and return type up to conversion) |
| `definition` | Initialize with different constructor arguments + manually manipulate instances per-variation |

# Propagation of variations

Nominal and varied actions are automatically carried forward during an analysis, meaning:

- Any dependent columns and selections evaluated from varied input columns will be varied.
- Any querys performed using varied columns and selections will be varied.

The propagation proceeds "in concurrent" and "transparently", meaning:

- If two actions each have a variation of the same name, they are in effect together.
- If one action has a variation while another doesn't, then the nominal is in effect for the latter.

![variation](../../assets/variation.png)