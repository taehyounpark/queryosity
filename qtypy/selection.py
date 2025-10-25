from abc import ABC, abstractmethod

from . import lazynode 
from .cpputils import find_cpp_identifiers

class selection(lazynode):

    def __init__(self, expr: str):
        super().__init__()

        self.expr = expr
        self.args = find_cpp_identifiers(expr)

        self.preselection_name = None

    def __str__(self):
        return f'{self.operation}({self.expr})'

    def at(self, preselection_name):
        self.preselection_name = preselection_name
        return self

    def __matmul__(self, preselection_name):
        return self.at(preselection_name)

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
            self.preselection = self.df.current_selection
        else:
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
    def __init__(self, expr: str):
        super().__init__(expr)

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
    def __init__(self, expr: str):
        super().__init__(expr)

    @property
    def operation(self):
        return 'weight'

    @property
    def value_type(self):
        return f'double'
