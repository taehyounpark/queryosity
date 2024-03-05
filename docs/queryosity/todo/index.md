---
title: todo
description:  A todo node instantiates a lazy action upon inputs from existing ones.
generator: doxide
---


# todo

**template &lt;typename Helper&gt; class todo : public dataflow::node, public concurrent::slotted&lt;Helper&gt;, public systematic::resolver&lt;todo&lt;Helper&gt;&gt;**


A todo node instantiates a lazy action upon inputs from existing ones.

:material-code-tags: **Template parameter** `Helper`
:    Helper class to instantiate the lazy action.
    


## Operators

| Name | Description |
| ---- | ----------- |
| [operator()](#operator_u0028_u0029) |  Shorthand for `evaluate()`. |

## Functions

| Name | Description |
| ---- | ----------- |
| [evaluate](#evaluate) |  Evaluate the column out of existing ones. |
| [fill](#fill) |  Fill query with input columns per-entry. |
| [book](#book) |  Book the query at a selection. |
| [book](#book) |  Book multiple query at multiple selections. |

## Operator Details

### operator()<a name="operator_u0028_u0029"></a>

!!! function "template &lt;typename... Args, typename V = Helper, std::enable_if_t&lt;column::template is_evaluatable_v&lt;V&gt; || selection::template is_applicable_v&lt;V&gt;, bool&gt; = false&gt; auto operator()(Args &amp;&amp;...columns) const -&gt; decltype(std::declval&lt;todo&lt;V&gt;&gt;().evaluate( std::forward&lt;Args&gt;(std::declval&lt;Args &amp;&amp;&gt;())...))"

    
    Shorthand for `evaluate()`.
    
    :material-code-tags: **Template parameter** `Args...`
    :    Input column types.
    
    :material-location-enter: **Parameter** `columns...`
    :    Input columns.
    
    :material-keyboard-return: **Return**
    :    Evaluated column.
    
    

## Function Details

### book<a name="book"></a>
!!! function "template &lt;typename Node&gt; auto book(Node &amp;&amp;selection) const"

    
    Book the query at a selection.
    
    :material-location-enter: **Parameter** `sel`
    :    Selection node at which query is counted/filled.
    
    :material-keyboard-return: **Return**
    :    The query booked at the selection.
    
    

!!! function "template &lt;typename... Sels&gt; auto book(Sels &amp;&amp;...sels) const"

    
    Book multiple query at multiple selections.
    
    :material-code-tags: **Template parameter** `Sels...`
    :    Selections.
    
    :material-location-enter: **Parameter** `sels...`
    :    selection nodes.
    
    :material-keyboard-return: **Return**
    :    `std::tuple` of queries booked at each selection.
    
    

### evaluate<a name="evaluate"></a>
!!! function "template &lt;typename... Nodes, typename V = Helper, std::enable_if_t&lt;queryosity::column::template is_evaluatable_v&lt;V&gt;, bool&gt; = false&gt; auto evaluate(Nodes &amp;&amp;...columns) const -&gt; decltype(std::declval&lt;todo&lt;V&gt;&gt;()._evaluate( std::forward&lt;Nodes&gt;(columns)...))"

    
    Evaluate the column out of existing ones.
    
    :material-location-enter: **Parameter** `columns`
    :    Input columns.
    
    :material-keyboard-return: **Return**
    :    Evaluated column.
    
    

### fill<a name="fill"></a>
!!! function "template &lt;typename... Nodes, typename V = Helper, std::enable_if_t&lt;queryosity::query::template is_bookable_v&lt;V&gt;, bool&gt; = false&gt; auto fill(Nodes &amp;&amp;...columns) const -&gt; decltype(std::declval&lt;todo&lt;V&gt;&gt;()._fill(std::declval&lt;Nodes&gt;()...))"

    
    Fill query with input columns per-entry.
    
    :material-location-enter: **Parameter** `columns`
    :    Input columns.
    
    :material-keyboard-return: **Return**
    :    Query filled with input columns.
    
    

