A `selection` is a decision made on an entry of either:

- A boolean as a `cut` to determine if the entry should be counted in a query or not.
- A floating-point value as a `weight` to assign a statistical significance to the entry.

In other words, selections themselves are a specific type of columns whose scalar values can be compounded:

- A series of two or more cuts is equivalent to their intersection, `&&`.
- A series of two or more weights is equivalent to their product, `*`.

A selection cutflow is built from the following kinds of connections between nodes:

- Compounding selections as stated above. Also, a cut can be applied from a weight and vice versa (see [User guide](../guide/selection.md)).
- Branching selections by applying more than one selection from a common node.
- Merging two selections, e.g. taking the union/intersection of two cuts.

<figure markdown="span">
  ![cutflow](../assets/cutflow.png)
  <figcaption>Example cutflow with compounded, branched, and merged selections.</figcaption>
</figure>
