
from ..query import bkpr

class series(bkpr):

    def __init__(self, column, dtype = 'double'):
        super().__init__()
        self.filled_columns = [[column]]
        self.dtype = dtype

    def fill(self, *columns):
        raise ValueError("series must be exactly one column")

    def __str__(self):
        return self.filled_columns[0][0] + ' → series'

    @property
    def get_call(self):
        return f'get(qty::query::output<qty::query::series<{self.dtype}>>())'

    @property
    def result_type(self):
        return f'std::vector<{self.dtype}>'
