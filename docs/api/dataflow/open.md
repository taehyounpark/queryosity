# `dataflow::open`

```cpp
template <typename DS, typename... Args> reader<DS> open(Args &&...args);
```

## Template parameters

| Template parameter | Description | Example
| :-- | :-- | :-- |
| `DS` | Dataset type | `ana::json` |
| `Args &&...` | Constructor argument type(s) for `DS` | `nlohmann::json` |

## Parameters

`args...` (in) - Constructor argument(s) for dataset.