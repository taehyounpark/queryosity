---
title: result_type
description: Minimal query with an output result.
generator: doxide
---


# result_type

**template &lt;typename T&gt; using result_type = T**



Minimal query with an output result.

@details This ABC should be used for actions that do not require any input
columns.



## Functions

| Name | Description |
| ---- | ----------- |
| [result](#result) | Create and return the result of the query. |
| [merge](#merge) | Merge the results from concurrent slots into one. |
| [finalize](#finalize) | Count an entry for which the booked selection has passed with its * weight. |

## Function Details

### finalize<a name="finalize"></a>
!!! function "virtual void finalize(unsigned int) final override"

    
    
    Count an entry for which the booked selection has passed with its
       * weight.
    
    :material-location-enter: **Parameter** `weight`
    :    The value of the weight at booked selection for the passed
        entry.
        
    

### merge<a name="merge"></a>
!!! function "virtual T merge(std::vector&lt;T&gt; const &amp;results) const = 0"

    
    
    Merge the results from concurrent slots into one.
    
    :material-location-enter: **Parameter** `results`
    :    Incoming results.
    
    :material-keyboard-return: **Return**
    :    The merged result.
    
    

### result<a name="result"></a>
!!! function "virtual T result() const = 0"

    
    
    Create and return the result of the query.
    
    :material-keyboard-return: **Return**
    :    The result.
    
    @detail The aggregation from each concurrent slot, which is returned by
    value, are collected into a list to be merged into one.
    
    

