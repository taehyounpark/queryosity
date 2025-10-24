from abc import ABC, abstractmethod

from . import lazynode 
from .cpputils import parse_cpp_expression

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
            "a": filter("x > 0"),
            "b": filter("y < 0"),   # a && b
            "c": filter("z != 0")   # a && b && c
        })

    .. code-block:: python

        # branching cuts, a -> (a && b, a && c)
        df.apply({
            "a": filter("x > 0"),
            "b": filter("y < 0"),          # a && b
            "c": at("a").filter("z != 0")  # a && c
        })
    """

    def __init__(self, preselection):
        self.preselection_name = preselection

    def filter(self, expr : str):
        return filter(expr, preselection=self.preselection_name)

    def weight(self, expr: str):
        return weight(expr, preselection=self.preselection_name)

class selection(lazynode):

    def __init__(self, expr: str, preselection : str = None):
        super().__init__()
        self.cpp_prefix += 'selection_'
        self.cpp_typename = 'auto'

        self.expr = expr
        self.args = parse_cpp_expression(expr)

        self.preselection_name = preselection

    def __str__(self):
        return f'{self.operation}({self.expr})'

    @property
    @abstractmethod
    def operation(self):
        pass

    @property
    @abstractmethod
    def value_type(self):
        pass

    @property
    def cpp_initialization(self):
        column_args = [arg for arg in self.args if arg in self.df.columns]
        lmbd_args = [f'{self.df.columns[arg].value_type} const& {arg}' for arg in column_args]
        lmbd_defn = '[](' + ', '.join(lmbd_args) + '){return (' + self.expr + ');}'
        lazy_args = [self.df.columns[arg].cpp_identifier for arg in column_args]

        if self.preselection_name is None:
            if self.df == self.df.current_selection: self.preselection_name = '--'
            else: self.preselection_name = self.df.current_selection.name
            self.preselection = self.df.current_selection
        else:
            self.preselection_name = self.preselection.name
            self.preselection = self.df.selections[self.preselection_name]

        return f'{self.preselection.cpp_identifier}.{self.operation}(qty::column::expression({lmbd_defn})).apply({", ".join(lazy_args)})'

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
    def __init__(self, expr: str, preselection : str = None):
        super().__init__(expr, preselection)

    @property
    def operation(self):
        return 'filter'

    @property
    def value_type(self):
        return 'bool'

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
    def __init__(self, expr: str, preselection : str = None):
        super().__init__(expr, preselection)

    @property
    def operation(self):
        return 'weight'

    @property
    def value_type(self):
        return f'double'
