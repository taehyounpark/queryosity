# Design goals

Queryosity has been purposefully designed for data analysis workflows in high-energy physics experiments prioritizing the following principles.

---

**Clarity and consistency.**

- The interface is a faithful representation of the computation task graph.
- The analysis logic written by Alice is readable to Bob and vice versa.

---

**Arbitrary data types.**

- If a dataset has rows, it can be queried.
- Column values can be ~~scalars or arrays~~ of non-trivial data types.
- Queries can output results of any data structure.

---

**Universal cutflow for selections.**

- A cut/weight is a boolean/floating-point selection of entries.
- Selections can be arbitrarily deep (compounded) or wide (branched).
- Queries performed under a given selection are populated with the same entries and weights.

---

**Optimal(maximal) efficiency(usage) of computational resources.**

- Never perform an action for an event unless needed.
- Partition the dataset and traverse over each sub-range in parallel.

---

**Built-in, generalized handling of systematic variations.**

- An experiment can be subject to $O(100)$ sources of systematic uncertainties.
- Applying systematic variations that are automatically propagated and processed in one dataset traversal is crucial for minimizing "time-to-insight".