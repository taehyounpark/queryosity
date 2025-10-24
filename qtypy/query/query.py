from .. import lazynode

class lazyquery(lazynode):

    def __init__(self, df, bkpr, sel):
        super().__init__()
        self.df = df
        self.bkpr = bkpr
        self.sel = sel

    @property
    def cpp_initialization(self) -> str:
        fill_calls = []
        for column_names in self.bkpr.filled_columns:
            fill_call = 'fill('
            fill_call += ', '.join([f'{self.df.columns[column_name].cpp_identifier}' for column_name in column_names])
            fill_call += ')'
            fill_calls.append(fill_call)
        at_call = f'at({self.sel.cpp_identifier})'
        return f'{self.df.cpp_identifier}.{self.bkpr.get_call}.{".".join(fill_calls)}.{at_call}'