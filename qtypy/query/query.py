from .. import lazy

class query(lazy):

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