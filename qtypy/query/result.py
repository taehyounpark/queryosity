import cppyy
import numpy as np

from ..cpputils import cpp_instantiable

class result(cpp_instantiable):
    def __init__(self, query):
        super().__init__()
        self.query = query
        self.name = f'result_{query.name}'

    @property
    def cpp_typename(self):
        return f'{self.query.defn.cpp_result_type} const &'

    @property
    def cpp_initialization(self):
        return f'{self.query.cpp_identifier}.{self.query.defn.cpp_result_call}'

    def result(self):
        return self.query.defn.py_result_wrapper(self.cpp_instance)