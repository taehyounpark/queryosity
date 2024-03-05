A `query` node defines:

- An arbitrary action to be executed under a particular selection, i.e. only if the cut has passed.
    -  (Optional) Take into account the selection weight.
- (Optional) Populate the result based on values of input columns of the entry.
- Output a result once the full dataset has been traversed.

Given a concrete definition of a query, it can be performed for entries passing any specified selections.

![agg_book_sels](../../assets/agg_book_sels.png)

Conversely, book queries from a selection node:

![sel_book_aggs](../../assets/sel_book_aggs.png)