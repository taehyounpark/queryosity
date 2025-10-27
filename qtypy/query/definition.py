from abc import ABC, abstractmethod

from ..node import query

class definition(ABC):

    def __init__(self):
        self.filled_columns : list[list[str]] = []
        pass

    def fill(self, *columns):
        self.filled_columns.append(columns)
        return self

    def at(self, selection):
        return query(self, selection)

    def __matmul__(self, selection):
        return self.at(selection)

    @property
    def py_result_wrapper(self):
        return lambda x: x

    @property
    @abstractmethod
    def cpp_get_call(self):
        """C++ code string to call ``dataflow::get()``"""
        pass

    @property
    def cpp_result_type(self):
        return 'auto'

    @property
    def cpp_result_call(self):
        return 'result()'