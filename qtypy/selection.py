import cppyy

from .cpp import cpp_binding 
from .cpp import parse_cpp_identifiers

class at:
    """
    Apply a selection using a specific preselection.

    By default, selections are applied sequentially, meaning each subsequent
    call compounds on the previous selection as its preselection. Use
    ``at(preselection_name)`` to override this behavior and explicitly set the
    preselection for a cut or weight.

    Examples
    --------
    .. code-block:: python

        # compounding cuts, a -> (a && b) -> (a && b && c)
        df.apply({
            "a": cut("x > 0"),
            "b": cut("y < 0"),   # a && b
            "c": cut("z != 0")   # a && b && c
        })

    .. code-block:: python

        # branching cuts, a -> (a && b, a && c)
        df.apply({
            "a": cut("x > 0"),
            "b": cut("y < 0"),          # a && b
            "c": at("a").cut("z != 0")  # a && c
        })
    """

    def __init__(self, preselection_name):
        self.preselection_name = preselection_name

    def cut(self, expr : str):
        return cut(expr, preselection=self.preselection_name)

    def weight(self, expr: str):
        return weight(expr, preselection=self.preselection_name)

class cut(cpp_binding):
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
    def __init__(self, expr: str, preselection : str = None):
        super().__init__()
        self.expr = expr
        self.args = parse_cpp_identifiers(expr)
        self.preselection = preselection

        self.cpp_prefix = '_df_cut'

    def instantiate(self, df):
        lmbd_args = [f'{df.columns[arg].value_type} const& {arg}' for arg in self.args]
        lmbd_defn = '[]'
        lmbd_defn += '(' + ', '.join(lmbd_args) + ')'
        lmbd_defn += '{'+f'return ({self.expr});'+'}'

        lazy_args = [f'{df.columns[arg].cpp_identifier}' for arg in self.args]

        presel = df.current_selection if self.preselection is None else df.selections[self.preselection]

        cppyy.cppdef(f'auto {self.cpp_identifier} = {presel.cpp_identifier}.filter(qty::column::expression({lmbd_defn})).apply({", ".join(lazy_args)});')

        self.value_type = f'qty::column::value_t<typename decltype({self.cpp_identifier})::action_type>'

class weight(cpp_binding):
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
    def __init__(self, expr: str, preselection : str = None):
        super().__init__()
        self.expr = expr
        self.args = parse_cpp_identifiers(expr)
        self.preselection = preselection

        self.cpp_prefix = '_df_weight'

    def instantiate(self, df):
        lmbd_args = [f'{df.columns[arg].value_type} const& {arg}' for arg in self.args]
        lmbd_defn = '[]'
        lmbd_defn += '(' + ', '.join(lmbd_args) + ')'
        lmbd_defn += '{'+f'return ({self.expr});'+'}'

        lazy_args = [f'{df.columns[arg].cpp_identifier}' for arg in self.args]

        presel = df.current_selection if self.preselection is None else df.selections[self.preselection]

        cppyy.cppdef(f'auto {self.cpp_identifier} = {presel.cpp_identifier}.weight(qty::column::expression({lmbd_defn})).apply({", ".join(lazy_args)});')

        self.value_type = f'qty::column::value_t<typename decltype({self.cpp_identifier})::action_type>'
