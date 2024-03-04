---
title: dataflow
description:  Main data analysis and query graph interface.
generator: doxide
---


# dataflow

**class dataflow**


Main data analysis and query graph interface.



## Functions

| Name | Description |
| ---- | ----------- |
| [dataflow](#dataflow) | Default constructor.  |
| [dataflow](#dataflow) | Constructor with one keyword argument. |
| [open](#open) | Open a dataset input. |

## Function Details

### dataflow<a name="dataflow"></a>
!!! function "dataflow()"

    
    
    Default constructor.
       
    
    
    

!!! function "template &lt;typename Kwd&gt; dataflow(Kwd kwarg)"

    
    
    Constructor with one keyword argument.
    
    :material-code-tags: **Template parameter** `Kwd`
    :    Keyword argument type.
    
    @details A keyword argument can be:
    - `queryosity::dataset::weight(float)`
    - `queryosity::dataset::limit(unsigned` int)
    - `queryosity::multithread::enable(unsigned` int)
    
    

### open<a name="open"></a>
!!! function "template &lt;typename DS&gt; auto open(dataset::input&lt;DS&gt; &amp;&amp;in) -&gt; dataset::opened&lt;DS&gt;"

    
    
    Open a dataset input.
    
    :material-code-tags: **Template parameter** `DS`
    :    Dataset input.
    
    :material-code-tags: **Template parameter** `Args...`
    :    Dataset input constructor arguments
    
    :material-keyboard-return: **Return**
    :    Opened dataset input
    
    

