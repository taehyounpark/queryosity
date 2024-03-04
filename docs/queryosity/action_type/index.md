---
title: action_type
description: Variations of a lazy action to be performed in an dataflow.
generator: doxide
---


# action_type

**template &lt;typename Act&gt; using action_type = typename lazy&lt;Act&gt;::action_type**



Variations of a lazy action to be performed in an dataflow.

:material-code-tags: **Template parameter** `T`
:    Input dataset type

:material-code-tags: **Template parameter** `U`
:    Actions to be performed lazily.

@details A `varied` node can be treated identical to a `lazy` one, except
that it contains multiple variations of the action as dictated by the
analyzer that propagate through the rest of the analysis.



