---
title: todo
description: A node that instantiates a lazy action.
generator: doxide
---


# todo

**template &lt;typename Bkr&gt; class todo : public dataflow::node, public concurrent::slotted&lt;Bkr&gt;, public systematic::resolver&lt;todo&lt;Bkr&gt;&gt;**



A node that instantiates a lazy action.

@details A todo node requires additional inputs to instantiate a lazy
action.

:material-code-tags: **Template parameter** `Bkr`
:    Booker that instantiates a lazy action.
    


## Operators

| Name | Description |
| ---- | ----------- |
| [operator()](#operator_u0028_u0029) | Shorthand for `evaluate()` and `apply()` * for column and selection respectively. |

## Functions

| Name | Description |
| ---- | ----------- |
| [evaluate](#evaluate) | Evaluate the column out of existing ones. |
| [fill](#fill) | Fill the query with input columns. |
| [book](#book) | Book the query at a selection. |
| [_evaluate](#_evaluate) | Evaluate a column definition out of nominal input columns  |
| [_evaluate](#_evaluate) | Evaluate a column definition out of at least one varied input * columns  |
| [_book](#_book) | Book an query at a nominal selection  |
| [_book](#_book) | Book an query at a varied selection  |

## Operator Details

### operator()<a name="operator_u0028_u0029"></a>

!!! function "template &lt;typename... Args, typename V = Bkr, std::enable_if_t&lt;column::template is_evaluatable_v&lt;V&gt; || selection::template is_applicable_v&lt;V&gt;, bool&gt; = false&gt; auto operator()(Args &amp;&amp;...columns) const -&gt; decltype(std::declval&lt;todo&lt;V&gt;&gt;().evaluate( std::forward&lt;Args&gt;(std::declval&lt;Args &amp;&amp;&gt;())...))"

    
    
    Shorthand for `evaluate()` and `apply()`
       * for column and selection respectively.
    
    :material-location-enter: **Parameter** `columns`
    :    The input columns.
    
    :material-keyboard-return: **Return**
    :    The evaluated/applied column/selection.
    
    

## Function Details

### _book<a name="_book"></a>
!!! function "template &lt;typename Node, typename V = Bkr, std::enable_if_t&lt;queryosity::query::template is_bookable_v&lt;V&gt; &amp;&amp; queryosity::is_nominal_v&lt;Node&gt;, bool&gt; = false&gt; auto _book(Node const &amp;sel) const -&gt; lazy&lt;query::booked_t&lt;V&gt;&gt;"

    
    
    Book an query at a nominal selection
       
    
    
    

!!! function "template &lt;typename Node, typename V = Bkr, std::enable_if_t&lt;queryosity::query::template is_bookable_v&lt;V&gt; &amp;&amp; queryosity::is_varied_v&lt;Node&gt;, bool&gt; = false&gt; auto _book(Node const &amp;sel) const -&gt; typename lazy&lt;query::booked_t&lt;V&gt;&gt;::varied"

    
    
    Book an query at a varied selection
       
    
    
    

### _evaluate<a name="_evaluate"></a>
!!! function "template &lt;typename... Nodes, typename V = Bkr, std::enable_if_t&lt;queryosity::column::template is_evaluatable_v&lt;V&gt; &amp;&amp; queryosity::has_no_variation_v&lt;Nodes...&gt;, bool&gt; = false&gt; auto _evaluate(Nodes const &amp;...columns) const -&gt; lazy&lt;column::template evaluated_t&lt;V&gt;&gt;"

    
    
    Evaluate a column definition out of nominal input columns
       
    
    
    

!!! function "template &lt;typename... Nodes, typename V = Bkr, std::enable_if_t&lt;queryosity::column::template is_evaluatable_v&lt;V&gt; &amp;&amp; queryosity::has_variation_v&lt;Nodes...&gt;, bool&gt; = false&gt; auto _evaluate(Nodes const &amp;...columns) const -&gt; typename lazy&lt;column::template evaluated_t&lt;V&gt;&gt;::varied"

    
    
    Evaluate a column definition out of at least one varied input
       * columns
       
    
    
    

### book<a name="book"></a>
!!! function "template &lt;typename Node&gt; auto book(Node &amp;&amp;selection) const"

    
    
    Book the query at a selection.
    
    :material-location-enter: **Parameter** `selection`
    :    Selection to be counted.
    
    :material-keyboard-return: **Return**
    :    The query booked at the selection.
    
    

### evaluate<a name="evaluate"></a>
!!! function "template &lt;typename... Nodes, typename V = Bkr, std::enable_if_t&lt;queryosity::column::template is_evaluatable_v&lt;V&gt;, bool&gt; = false&gt; auto evaluate(Nodes &amp;&amp;...columns) const -&gt; decltype(std::declval&lt;todo&lt;V&gt;&gt;()._evaluate( std::forward&lt;Nodes&gt;(columns)...))"

    
    
    Evaluate the column out of existing ones.
    
    :material-location-enter: **Parameter** `columns`
    :    Input columns.
    
    :material-keyboard-return: **Return**
    :    Evaluated column.
    
    

### fill<a name="fill"></a>
!!! function "template &lt;typename... Nodes, typename V = Bkr, std::enable_if_t&lt;queryosity::query::template is_bookable_v&lt;V&gt;, bool&gt; = false&gt; auto fill(Nodes &amp;&amp;...columns) const -&gt; decltype(std::declval&lt;todo&lt;V&gt;&gt;().fill_query( std::declval&lt;Nodes&gt;()...))"

    
    
    Fill the query with input columns.
    
    :material-location-enter: **Parameter** `columns`
    :    Input columns
    
    :material-keyboard-return: **Return**
    :    The query filled with input columns.
    
    

