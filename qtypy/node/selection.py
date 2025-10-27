from abc import ABC, abstractmethod

from ..cpp import find_cpp_identifiers
from .lazy import lazy 

class selection(lazy):

    def __init__(self, expr: str):
        super().__init__()

        self.expr = expr
        self.args = find_cpp_identifiers(expr)

    def __str__(self):
        return f'{self.operation}({self.expr})'

    @property
    @abstractmethod
    def operation(self):
        pass

    @property
    @abstractmethod
    def cpp_value_type(self):
        pass

    @property
    def cpp_initialization(self):
        column_args = [arg for arg in self.args if arg in self.df.columns]
        lmbd_args = [f'{self.df.columns[arg].cpp_value_type} const& {arg}' for arg in column_args]
        lmbd_defn = '[](' + ', '.join(lmbd_args) + '){return (' + self.expr + ');}'
        lazy_args = [self.df.columns[arg].cpp_identifier for arg in column_args]

        return f'{self.df.current_selection.cpp_identifier}.{self.operation}(qty::column::expression({lmbd_defn})).apply({", ".join(lazy_args)})'

    def contextualize(self, df, name):
        # registered also as a column
        if name in df.columns:
            raise ValueError("column already exists")
        df.columns[name] = self
        # register as a selection
        if name in df.selections:
            raise ValueError("selection already exists")
        df.selections[name] = self

        self.df = df
        self.instantiate()

        # move dataflow to current selection
        df.current_selection = self