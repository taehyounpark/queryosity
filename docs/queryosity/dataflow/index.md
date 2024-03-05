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
| [dataflow](#dataflow) |  Constructor with (up to) three keyword arguments, which can be one of the following: - `queryosity::multithread::enable(unsigned int)` - `queryosity::dataset::limit(unsigned int)` - `queryosity::dataset::weight(float)`  |
| [open](#open) |  Open a dataset input. |

## Function Details

### dataflow<a name="dataflow"></a>
!!! function "dataflow()"

    
    
    Default constructor.
       
    
    
    

!!! function "template &lt;typename Kwd1, typename Kwd2, typename Kwd3&gt; dataflow(Kwd1 kwarg1, Kwd2 kwarg2, Kwd3 kwarg3)"

    
    Constructor with (up to) three keyword arguments, which can be one of the
    following:
    
    - `queryosity::multithread::enable(unsigned int)`
    - `queryosity::dataset::limit(unsigned int)`
    - `queryosity::dataset::weight(float)`
    
    
    

### open<a name="open"></a>
!!! function "template &lt;typename DS&gt; auto open(dataset::input&lt;DS&gt; &amp;&amp;in) -&gt; dataset::opened&lt;DS&gt;"

    
    Open a dataset input.
    
    :material-code-tags: **Template parameter** `DS`
    :    Dataset input.
    
    :material-code-tags: **Template parameter** `Args...`
    :    Dataset input constructor arguments
    
    :material-keyboard-return: **Return**
    :    Opened dataset input
    
    

