from ..node import selection 

class weight(selection):
    """
    Represents a floating-point weight applied to a row, considered only when
    a cut is passed.

    A ``weight`` evaluates a numerical expression for each row representing
    its statistical significance. Weights compound by multiplication onto
    existing preselection(s).

    Parameters
    ----------
    expr : str
        An expression returning a floating-point value for each row.
    preselection : str, optional
        Name of a prior selection (cut) to restrict the weight application.
    """
    def __init__(self, expr: str):
        super().__init__(expr)

    @property
    def operation(self):
        return 'weight'

    @property
    def cpp_value_type(self):
        return f'double'
