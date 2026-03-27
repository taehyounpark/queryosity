
import numpy as np

from .definition import definition

class counter(definition):

    def __init__(self):
        super().__init__()

    def __str__(self):
        # xax = 
        return "[entries, count, error]"

    @property
    def cpp_result_type(self):
        return "qty::selection::count_t"

    @property
    def cpp_result_call(self):
        return 'result()'

    @property
    def cpp_get_call(self):
        return 'get(qty::query::output<qty::selection::counter>())'
