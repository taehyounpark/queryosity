from ..node import selection 

class filter(selection):
    """
    Represents a floating-point weight applied to a row, considered only
    when a cut is passed.

    A ``weight`` evaluates a numerical expression for each row representing
    its statistical significance. Weights compound by multiplication onto
    existing preselection(s).
    
    Parameters
    ----------
    expr : str
        A boolean expression to evaluate on the dataset rows.
    preselection : str, optional
        Name of a prior selection to use as the base for this cut.
    """
    def __init__(self, expr: str):
        super().__init__(expr)

    @property
    def operation(self):
        return 'filter'

    @property
    def cpp_value_type(self):
        return 'bool'