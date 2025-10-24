import cppyy
import numpy as np

from .. import lazynode

class lazyresult(lazynode):
    def __init__(self, query):
        super().__init__()
        self.query = query
        self.name = f'result_{query.name}'
        self.cpp_typename = f'{self.query.bkpr.result_type} const &'

    @property
    def cpp_initialization(self):
        return f'{self.query.cpp_identifier}.{self.query.bkpr.result_call}'

    def get(self):
        return self.cpp_instance