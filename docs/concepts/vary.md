To perform a sensitivity analysis means to determine how variations in the input of a system affect its output.

A **systematic variation** constitutes a __change in a column value that affects the outcome of selections and queries__.
There is built-in support to handle these inside a single dataflow object, which offer the following benefits:

- Guarantee that only one variation is in effect at a time versus a nominal, by construction.
- Eliminate runtime overhead associated with repeated dataset traversals.

# Propagation of variations

A varied node, can be treated analogously as a single node, only now that any downstream actions will contain its nominal+variations. *i.e. The rest of the dataflow graph need not be edited!* 

- Any column definitions and selections evaluated out of varied input columns will be varied.
- Any queries performed filled with varied input columns and/or at varied selections will be varied.

The propagation proceeds "in lockstep" and "transparently", meaning:

- If two actions each have a variation of the same name, they are in effect together.
- If one action has a variation while another doesn't, then the nominal is in effect for the latter.

![variation](../../assets/variation.png)