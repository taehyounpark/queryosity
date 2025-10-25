import cppyy
import numpy as np

from .. import lazynode

class lazyresult(lazynode):
    def __init__(self, query):
        super().__init__()
        self.query = query
        self.name = f'result_{query.name}'
        self.cpp_typename = f'{self.query.defn.cpp_result_type} const &'

    @property
    def cpp_initialization(self):
        return f'{self.query.cpp_identifier}.{self.query.defn.cpp_result_call}'

    def result(self):
        return self.query.defn.py_result_wrapper(self.cpp_instance)