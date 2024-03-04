---
title: queryosity
description: 
generator: doxide
---


# queryosity



:material-package: [column](column/index.md)
:   

## Types

| Name | Description |
| ---- | ----------- |
| [action](action/index.md) | abstract base class with initialization, execution, finalization steps  |
| [action_type](action_type/index.md) | Variations of a lazy action to be performed in an dataflow. |
| [dataflow](dataflow/index.md) |  Main data analysis and query graph interface. |
| [result_type](result_type/index.md) | Minimal query with an output result. |
| [todo](todo/index.md) | A node that instantiates a lazy action. |
| [vartup_type](vartup_type/index.md) | Counter output to be filled with columns using arbitrary logic. |
| [vartuple_type](vartuple_type/index.md) | Column with user-defined return value type and evaluation * dataset. |
| [vartuple_type](vartuple_type/index.md) | Representation of multipile columns, out of which derived quantites * can be calculated.  |

## Functions

| Name | Description |
| ---- | ----------- |
| [calculation](#calculation) | Calculate a column value for each dataset entry. |
| [computation](#computation) |  Computation graph of columns. |
| [varied](#varied) | Varied version of a todo node. |

## Function Details

### calculation<a name="calculation"></a>
!!! function "template &lt;typename Val&gt; calculation()"

    
    
    Calculate a column value for each dataset entry.
    
    :material-code-tags: **Template parameter** `Val`
    :    Column value type.
    
    @details A calculation is performed once per-entry (if needed) and its value
    is stored for multiple accesses by downstream actions within the entry.
    The type `Val` must be *CopyConstructible* and *CopyAssignable*.
    
    

### computation<a name="computation"></a>
!!! function "computation() = default"

    
    Computation graph of columns.
    
    

### varied<a name="varied"></a>
!!! function "template &lt;typename Bld&gt; varied(todo&lt;Bld&gt; &amp;&amp;nom)"

    
    
    Varied version of a todo node.
    
    @details A todo varied action is functionally equivalent to a todo
    node, except that it contains multiple nodes corresponding to nominal and
    systematic variations.
    
    

