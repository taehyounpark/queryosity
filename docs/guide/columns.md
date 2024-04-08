# Computing quantities

New columns can be computed out of existing ones by calling `queryosity::dataflow::define()` with the appropriate argument, or operators between the underlying value types.

:::{admonition} Requirements on column value type
:class: important
- {{DefaultConstructible}}.
- {{CopyAssignable}} or {{MoveAssignable}}.
:::

## Basic operations

```cpp
// constants columns do not change per-entry
auto zero = df.define(column::constant(0));
auto one = df.define(column::constant(1));
auto two = df.define(column::constant(2));

// binary/unary operators
auto three = one + two;
auto v_0 = v[zero];
// reminder: actions are *lazy*, i.e. no undefined behaviour (yet)

// can be re-assigned as long as value type remains unchanged
two = three - one;
```

## Custom expressions

Any C++ Callable object (function, functor, lambda, etc.) can be used to evaluate a column. Input columns to be used as arguments to the function should be provided separately.

```cpp
auto s_length = df.define(
    column::expression([](const std::string &txt) { return txt.length(); }))(s);
```
:::{tip}
Pass large values by `const &` to avoid expensive copies.
:::

## Custom definitions

A column can also be computed through a custom column definition, which enables full control over its

- Customization: user-defined constructor arguments and member variables/functions.
- Optimization: the computation of each input column is deferred until its value is invoked.

:::{admonition} Thread-safety requirements
:class: caution
- Each instance of a custom definition remain in a thread-safe state.
- Its constructor arguments must be {{CopyConstructible}}.
:::
