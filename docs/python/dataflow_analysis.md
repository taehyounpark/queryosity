# `@dataflow.analysis`

Often times, a data analysis step is composed of numerous individual actions to be performed in a dataflow.
A `@dataflow.analysis` block allows users to compose "blocks" of analysis logic that:

1. Ensures that all associated steps are specified in the correct order and relationship to one another.
2. Can be reused to perform the same steps with small differences in details depending on individual cases.

For example, consider applying a preselection that requires an event to pass any one of the specified triggers:

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

This block can now be re-configured & used with a different list of triggers, should it change in the future.

Invoking a `@dataflow.analysis` block via the `df | my_analysis()` operator will return back out the dataflow regardless of what is returned by the block.
Therefore, multiple blocks can be chained in sequence:

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
:::{tip}
Note that even though the signal region block is invoked before the control regions, the subsequent control region block is not affected as one can always to re-navigate the cutflow structure to the correct position.
:::

If one wishes a block to return some outputs, i.e. perform queries, one can use the `>>` operator:

```python
@dataflow.analysis
def get_cutflow_report(df, selections):
    return {
        sel : df @ sel >> count()
        for sel in selections
    }

yields = df >> get_cutflow_report([
    "preselection",
    "jet_requirements",
    "lep_requirements",
    "signal_region_selections"
])

print(yields)
```

```python
@dataflow.analysis
def get_all_distributions_at_selection(df, selection, histograms):
    """
    Get histograms of all variables under a particular selection
    """
    return [
        df @ selection >> hist
        for hist in histograms
    ]

df >> get_all_distributions_at_selection("base", [hist().fill(), hist().fill(), ...])

@dataflow.analysis
def get_distribution_at_all_selections(df, selections, histogram):
    """
    Get histograms of all variables under a particular selection
    """
    return [
        df @ sel >> histogram
        for sel in selections
    ]

df >> get_distribution_at_all_selections(["selection", ...], hist().fill())

@dataflow.analysis
def get_all_distribution_at_all_selections(df, selections, histograms):
    return {
        sel : [
            df @ sel >> hist
            for hist in histograms
        ]
        for sel in selections
    }

results = df >> 
```