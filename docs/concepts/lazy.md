Lazy actions are not executed until and unless the result of a query is needed.
This triggers the dataset traversal to populate the results of all queries specified up to that point in the dataflow graph.

Inside each entry, the eagerness of actions are as follows:

1. A query is executed only if its associated selection passes its selection.
2. A selection is evaluated only if all of its prior selections in the cutflow have passed.
3. A column is evaluated only if it is needed to determine any of the above.