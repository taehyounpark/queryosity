---
title: action_type
description:  Variations of a lazy action to be performed in an dataflow.
generator: doxide
---


# action_type

**template &lt;typename Act&gt; using action_type = typename lazy&lt;Act&gt;::action_type**


Variations of a lazy action to be performed in an dataflow.

:material-code-tags: **Template parameter** `Action`
:    The action.
    


## Functions

| Name | Description |
| ---- | ----------- |
| [filter](#filter) |  Apply a filter. |

## Function Details

### filter<a name="filter"></a>
!!! function "template &lt;typename Col, typename V = Act, std::enable_if_t&lt;queryosity::is_selection_v&lt;V&gt;, bool&gt; = false&gt; auto filter(Col const &amp;col) -&gt; typename lazy&lt;selection::node&gt;::varied"

    
    Apply a filter.
    
    

