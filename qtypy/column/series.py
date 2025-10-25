
from ..query import definition

import numpy as np

class series(definition):

    def __init__(self, column, dtype = 'double'):
        super().__init__()
        self.filled_columns = [[column]]
        self.dtype = dtype

    def fill(self, *columns):
        raise ValueError("series must be exactly one column")

    def __str__(self):
        return self.filled_columns[0][0] + ' → series'

    @property
    def cpp_get_call(self):
        return f'get(qty::query::output<qty::query::series<{self.dtype}>>())'

    @property
    def cpp_result_type(self):
        return f'std::vector<{self.dtype}>'

    @property
    def py_result_wrapper(self):
        return np.asarray
