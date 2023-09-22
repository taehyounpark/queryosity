## Cut versus weight

Entries in a dataset can be filtered by a `selection`, which is associated with a decision:

- If the decision is a boolean, it is a `cut` that determines whether to ignore the entry all-together.
- If the decision is a float-point value, it is a `weight` that assigns a statistical significance to the entry.

A selection can be applied by providing the column that corresponds to the decision value:

```cpp

  auto always_true = df.constant(true);
  auto all_entries = df.filter("all_entries")(always_true);

```
They can be applied in sequence to compounds them.

```cpp

  auto  = 

```
