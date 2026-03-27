from abc import ABC, abstractmethod

from ..cpp import find_cpp_identifiers_and_replace_with_values
from .lazy import lazy 

class selection(lazy):

    def __init__(self, column_spec):
        super().__init__()
        self.column_spec = column_spec
        self.previous_selection_name = None

    def __str__(self):
        return f'{self.operation}({self.column_spec})'

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
        self.df.columns[self.name].cpp_identifier

        return f'{self.df.current_selection.cpp_identifier}.{self.operation}({self.df.columns[self.name].cpp_identifier})'

    def _contextualize(self, df, name):

        # remember previous selection
        self.name = name
        self.previous_selection_name = df.current_selection_name

        # link to dataflow and JIT
        self.df = df

        self.df.compute({self.name : self.column_spec})

        self._instantiate()
        # register as a selection (its main job)
        if name in df.selections:
            raise ValueError("selection already exists")
        df.selections[name] = self

        # move dataflow to current selection
        df.current_selection_name = name
        df.current_selection = self