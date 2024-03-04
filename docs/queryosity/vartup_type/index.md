---
title: vartup_type
description: Counter output to be filled with columns using arbitrary logic.
generator: doxide
---


# vartup_type

**template &lt;typename T, typename... Obs&gt; using vartup_type = std::tuple&lt;column::variable&lt;Obs&gt;...&gt;**



Counter output to be filled with columns using arbitrary logic.

:material-code-tags: **Template parameter** `Obs...`
:    Input column data types.
    


## Functions

| Name | Description |
| ---- | ----------- |
| [fill](#fill) | Perform the counting action for an entry. |

## Function Details

### fill<a name="fill"></a>
!!! function "virtual void fill(column::observable&lt;Obs&gt;... observables, double w) = 0"

    
    
    Perform the counting action for an entry.
    
    :material-location-enter: **Parameter** `observables`
    :    The `observable` of each input column.
    
    :material-location-enter: **Parameter** `weight`
    :    The weight value of the booked selection for the passed
        entry.
    
    @details This action is performed N times for a passed entry, where N is
    the number of `fill` calls made to its `lazy` action, each with its the
    set of input columns as provided then.
    
    

