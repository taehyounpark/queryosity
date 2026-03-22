# `@dataflow.analysis`

Often times, a data analysis step is composed of numerous individual actions to be performed in a dataflow.
A `@dataflow.analysis` block allows users to compose "blocks" of analysis logic that:

1. Ensures that all associated steps are specified in the correct order and relationship to one another.
2. Can be reused to perform the same steps with small differences in details depending on individual cases.

For example,

```py
@dataflow.analysis
def apply_preselection(df, conditions):

    # read trigger variables from n-tuple
    for trig in conditions:
        df[trig] = dataset.column(trig, dtype='bool')

    df & {"preselection" : "||".join([trig for trig in conditions])}

conditions = ["", "", ""]
df | apply_preselection(conditions)
```

Within

Invoking a `@dataflow.analysis` block via the `df | user_block()` operator will *always* return the dataflow graph.
Therefore, multiple blocks can be "chained" one after the other:

```py
@dataflow.analysis
def apply_signal_region(df):

    df & {"mH_SR": "m4l >= mH_min && m4l < mH_max"}

@dataflow.analysis
def apply_control_region(df):

    base = (df @ "preselection")
    base & {"mH_CR_lo": "m4l < mH_min"}
    base & {"mH_CR_hi": "m4l >= mH_max"}

(
df
| apply_signal_region()
| apply_control_region()
)
    
```

Note that even though the signal region block is invoked before the control regions, the subsequent control region block is not affected as one can always use `df @ "sel"` to re-navigate the cutflow structure to the correct position.