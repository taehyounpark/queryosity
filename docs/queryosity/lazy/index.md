---
title: lazy
description:  A todo node instantiates a lazy action upon inputs from existing lazy actions.
generator: doxide
---


# lazy

**template &lt;typename Action&gt; class lazy : public dataflow::node, public concurrent::slotted&lt;Action&gt;, public systematic::resolver&lt;lazy&lt;Action&gt;&gt;, public result_if_aggregation&lt;Action&gt;**


A todo node instantiates a lazy action upon inputs from existing lazy
actions.

:material-code-tags: **Template parameter** `Bkr`
:    Helper class that instantiates a lazy action.
    


