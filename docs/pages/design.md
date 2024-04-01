@page design Design goals
<!-- @tableofcontents -->

`queryosity` has been purposefully designed for data analysis workflows in high-energy physics experiments prioritizing the following principles.

@section design-clear-interface Clear interface above all else.

- Provide a faithful, one-to-one correspondence between the description of the analysis logic by the interface and its underlying graph(s) of tasks being performed.
- The analysis code written by Alice must be readable and understandable to Bob, and vice versa.

@section design-arbitrary-data Arbirary data types.

- Many "columns" are not POD: they are of non-trivial data types containing nested properties, links to data of other types, etc. The interface for handling these columns should be front-and-center.
- If a dataset has rows, or "events", the library should be able to run over it.
- Output results of any data structure as desired.

@section design-cutflow Unified cutflow for cuts and weights.

- There is exactly one difference between a decision to (1) accept an event (cut), or (2) assign a statistical significance to it (weight): one is a yes-or-no, and the other is a number.
- Selections are defined individually, then connected through a "cutflow" that is as deep (compounded selections) or wide (branched selections) as needed.
- Whenever a particular selection is in effect for an event, all queries are consistently populated with the same cut and weight.

@section design-performance Optimal(maximal) efficiency(usage) of computational resources.

- Never perform an action for an event unless needed.
- The dataset is partitioned, and the traversal over each sub-range is parallelized.

@section design-systematic-variations Built-in, generalized handling of systematic variations.

- An experiment can be subject to @f$ O(100) @f$ sources of "systematic uncertainties".
- Applying systematic variations that are (1) specified once and automatically propagated, and (2) processed all at once in one dataset traversal, are crucial in minimizing "time-to-insight".

@see @ref conceptual 