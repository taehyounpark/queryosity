All operations within a dataflow

1. A aggregation will perform its action only if its booked selection has passed its cut.

2. A selection will evaluate its cut decision only if all upstream selections in the chain have passed, and its weight value only if the cut passes.

4. A column value will be evaluated only if it is needed to perform any of the above.

<!-- 1. The computation of ($p_\text{T}^H$ and $m_{\ell\ell}$) accesses (i.e. requires) the first 2 elements of the $\{p_{\text{T}}^{\ell}\}$ vector.

2. The histograms that fill those quantities were booked under a $n_\ell = 2$ selection.

3. The computation is never triggered for entries for which the vector has less than 2 entries. -->
