A sensitivity analysis means to study how changes in the input of a system affect its output. In the context of dataset queries, a **systematic variation** constitutes a __change in a column value that affects the outcome of selections and queries__.

In a dataflow, variations are specified *once* and then *propagated* through task graphs, eliminates code redundancy i.e. room for human error.
Also, processing variations in a single dataflow eliminates unnecessary repetitions of the dataset traversal and the associated runtime overhead.

# Propagation of systematic variations

Encapsulating the nominal and variations of an action creates a `varied` node in which each variation is mapped by the name of its associated systematic variation.

A varied node can be treated functionally identical to a nominal-only one, with all nominal+variations being propagated underneath:

- Any column definitions and selections evaluated out of varied input columns will be varied.
- Any queries performed filled with varied input columns and/or at varied selections will be varied.

The propagation proceeds in the following fashion:

- **Lockstep.** If two actions each have a variation of the same name, they are in effect together.
- **Transparent.** If only one action has a given variation, then the nominal is in effect for the other.


<figure markdown="span">
  ![variation](../../assets/variation.png)
  <figcaption>Propagation of systematic variations in \(z = f(x,y)\).</figcaption>
</figure>
