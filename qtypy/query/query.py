from abc import ABC, abstractmethod

from .. import lazynode

class definition(ABC):

    def __init__(self):
        self.filled_columns : list[list[str]] = []
        pass

    def fill(self, *columns):
        self.filled_columns.append(columns)
        return self

    def at(self, selection):
        return lazyquery(self, selection)

    def __matmul__(self, selection):
        return self.at(selection)

    @property
    def cpp_result_call(self):
        return 'result()'

    @property
    def py_result_wrapper(self):
        return lambda x: x

    @property
    def cpp_result_type(self):
        return 'auto'

    @property
    @abstractmethod
    def cpp_get_call(self):
        """C++ code string to call ``dataflow::get()``"""
        pass

class lazyquery(lazynode):

    def __init__(self, definition, booked_selection):
        super().__init__()
        self.defn = definition
        self.booked_selection = booked_selection

    @property
    def cpp_initialization(self) -> str:
        cpp_fill_calls = []
        for column_names in self.defn.filled_columns:
            fill_call = 'fill('
            fill_call += ', '.join([f'{self.df.columns[column_name].cpp_identifier}' for column_name in column_names])
            fill_call += ')'
            cpp_fill_calls.append(fill_call)
        at_call = f'at({self.df.selections[self.booked_selection].cpp_identifier})'
        return f'{self.df.cpp_identifier}.{self.defn.cpp_get_call}.{".".join(cpp_fill_calls)}.{at_call}'