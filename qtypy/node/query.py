from ..cpp import cpp_binding
from .lazy import lazy

class query(lazy):

    def __init__(self, defn):
        super().__init__()
        self.defn = defn

    def _contextualize(self, df):
        self.df = df
        self.booked_selection = df.current_selection
        self.booked_selection_name = df.current_selection_name
        self._instantiate()
        df.queries.append(self)

    @property
    def cpp_initialization(self) -> str:
        cpp_fill_calls = []
        for column_names in self.defn.filled_columns:
            fill_call = 'fill('
            fill_call += ', '.join([f'{self.df.columns[column_name].cpp_identifier}' for column_name in column_names])
            fill_call += ')'
            cpp_fill_calls.append(fill_call)
        at_call = f'at({self.booked_selection.cpp_identifier})'
        return f'{self.df.cpp_identifier}.{self.defn.cpp_get_call}.{".".join(cpp_fill_calls)}.{at_call}'