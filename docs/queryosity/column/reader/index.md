---
title: reader
description: Read columns from a dataset.
generator: doxide
---


# reader

**template &lt;typename T&gt; class reader : public cell&lt;T&gt;**



Read columns from a dataset.

:material-code-tags: **Template parameter** `T`
:    column data type.
    


## Functions

| Name | Description |
| ---- | ----------- |
| [read](#read) |  Read the value of the column at current entry. |

## Function Details

### read<a name="read"></a>
!!! function "virtual const_reference read(unsigned int, unsigned long long) const = 0"

    
    Read the value of the column at current entry.
    
    :material-location-enter: **Parameter** `slot`
    :    Multithreaded slot enumerator.
    
    :material-location-enter: **Parameter** `entry`
    :    Dataset global entry enumerator.
    
    :material-keyboard-return: **Return**
    :    Column value at current entry.
    
    

