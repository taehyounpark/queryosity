from abc import ABC, abstractmethod

from ..cpp import find_cpp_identifiers_and_replace_with_values
from .lazy import lazy 

class selection(lazy):

    def __init__(self, expr: str):
        super().__init__()
        self.expr = expr

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
        # only keep args that exist in columns
        column_args, expr = find_cpp_identifiers_and_replace_with_values(self.expr, set(self.df.columns))
        lmbd_args = [
            f'qty::column::observable<{self.df.columns[arg].cpp_value_type}> {arg}'
            for arg in column_args
        ]

        lmbd_defn = '[](' + ', '.join(lmbd_args) + '){return (' + expr + ');}'
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