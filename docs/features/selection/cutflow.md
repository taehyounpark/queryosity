A `selection` applied on entries refers to either:

- A boolean decision as a `cut` to determine if the entry should be considered or not.
- A floating-point value as a `weight` to assign a statistical significance to the entry.

In general, an arbitrary sequence of selections are applied, such that they are compounded:

- A series of two or more cuts is equivalent to the logical and of all of them.
- A series of two or more weights is equivalent to the product of all of them.

Finally, they may be applied in any mixed order (e.g. cut then weight, or weight then cut), and any number of arbitrary selections may be applied a common pre-selection.
Consider the following example:

1. Start out from some common preselection of entries.
2. Branch out to (left) apply a weight to all preselected entries, or (right) apply an additional cut.
3. On the right, apply a weight whose value is only defined for entries passing the upstream cut.
4. On the left, apply three different (not required to be orthogonal) cuts A, B, and C.
5. On each of A, B, and C, apply a common cut D.

This logical structure is referred to as a "cutflow" which can be represented as an n-ary tree:

```mermaid
graph TB
  A([Cut]) --> B([Weight W]);

  B --> C([Cut A]);
  C --> C2([Cut D]);

  B --> D([Cut B]);
  D --> D2([Cut D]);

  B --> H([Cut C]);
  H --> I([Cut D]);

  A --> F([Cut X]);
  F --> G([Weight Y]);
```

A good data analysis library should be able to specify any such non-trivial cutflows.