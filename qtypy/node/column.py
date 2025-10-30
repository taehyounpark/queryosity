from .lazy import lazy

from abc import abstractmethod

class column(lazy):
    def __init__(self):
        super().__init__()

    @property
    def cpp_type(self) -> str:
        return 'auto'

    @property
    def cpp_value_type(self) -> str:
        return f'qty::column::value_t<typename decltype({self.cpp_identifier})::action_type>'

    def contextualize(self, df, name):
        self.df = df
        self.instantiate()
        df.columns[name] = self