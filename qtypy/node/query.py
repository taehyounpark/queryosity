from ..cpp import cpp_binding
from .lazy import lazy

class query(lazy):

    def __init__(self, defn):
        super().__init__()
        self.defn = defn

    def contextualize(self, df):
        self.df = df

    @property
    def cpp_initialization(self) -> str:
        cpp_fill_calls = []
        for column_names in self.defn.filled_columns:
            fill_call = 'fill('
            fill_call += ', '.join([f'{self.df.columns[column_name].cpp_identifier}' for column_name in column_names])
            fill_call += ')'
            cpp_fill_calls.append(fill_call)
        at_call = f'at({self.df.current_selection.cpp_identifier})'
        return f'{self.df.cpp_identifier}.{self.defn.cpp_get_call}.{".".join(cpp_fill_calls)}.{at_call}'

class result(cpp_binding):
    def __init__(self, query):
        super().__init__()
        self.query = query
        self.name = f'result_{query.name}'

    @property
    def cpp_type(self):
        return f'{self.query.defn.cpp_result_type} const &'

    @property
    def cpp_initialization(self):
        return f'{self.query.cpp_identifier}.{self.query.defn.cpp_result_call}'

    def result(self):
        return self.query.defn.py_result_wrapper(self.cpp_instance)