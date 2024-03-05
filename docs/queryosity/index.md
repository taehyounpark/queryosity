---
title: queryosity
description: 
generator: doxide
---


# queryosity



## Types

| Name | Description |
| ---- | ----------- |
| [action_type](action_type/index.md) |  Variations of a lazy action to be performed in an dataflow. |
| [dataflow](dataflow/index.md) |  Main data analysis and query graph interface. |
| [lazy](lazy/index.md) |  A todo node instantiates a lazy action upon inputs from existing lazy actions. |
| [todo](todo/index.md) |  A todo node instantiates a lazy action upon inputs from existing ones. |

## Functions

| Name | Description |
| ---- | ----------- |
| [varied](#varied) | Varied version of a todo node. |

## Function Details

### varied<a name="varied"></a>
!!! function "template &lt;typename Bld&gt; varied(todo&lt;Bld&gt; &amp;&amp;nom)"

    
    
    Varied version of a todo node.
    
    @details A todo varied action is functionally equivalent to a todo
    node, except that it contains multiple nodes corresponding to nominal and
    systematic variations.
    
    

