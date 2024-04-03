@page design Design goals
<!-- @tableofcontents -->

`queryosity` has been purposefully designed for data analysis workflows in high-energy physics experiments prioritizing the following principles.

@section design-clear-interface Clarity and consistency above all else.

- The interface should be a faithful representation of the analysis task graph.
- The analysis code written by Alice should be readable and understandable to Bob, and vice versa.

@section design-arbitrary-data Arbitrary data types.

- Many "columns" are not trivial: they can contain nested properties, links to other data, etc.
- If a dataset has rows, or "events", the library should be able to run over it.
- Output results of any data structure as desired.

@section design-cutflow Unified cutflow for cuts and weights.

- There is only one difference between (1) accepting an event (cut), or (2) assigning a statistical significance to it (weight): one is a yes-or-no, and the other is a number.
- Selections can be arbitrarily deep (compounded selections) or wide (branched selections).
- Whenever a particular selection is in effect for an event, all queries are populated with the same entries and weights.

@section design-performance Optimal(maximal) efficiency(usage) of computational resources.

- Never perform an action for an event unless needed.
- Partition the dataset and traverse over each sub-range in parallel.

@section design-systematic-variations Built-in, generalized handling of systematic variations.

- An experiment can be subject to @f$ O(100) @f$ sources of "systematic uncertainties".
- Applying systematic variations that are (1) specified once and automatically propagated, and (2) processed all at once in one dataset traversal, is crucial for minimizing "time-to-insight".

@see @ref conceptual 