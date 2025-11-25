from ..node import column

from functools import cached_property

class definition(column):
    """
    Column defined by a one-line JIT-compiled C++ expression.

    Parameters
    ----------
    defn: str
    Name of the C++ function with observable arguments
    """
    def __init__(self, defn: str):
        super().__init__()
        self.defn = defn

    def __str__(self):
        return self.defn

    def __call__(self, *args):
        self.args = args
        return self

    @property
    def cpp_value_type(self):
        return f'qty::column::value_t<typename decltype({self.cpp_identifier})::action_type>'

    @property
    def cpp_initialization(self):
        lazy_args = [self.df.columns[col_name].cpp_identifier for col_name in self.args]

        return """{df_id}.define(qty::column::expression({defn})).evaluate({args})""".format(
            this_id=self.cpp_identifier,
            df_id=self.df.cpp_identifier,
            defn=self.defn,
            args=', '.join(lazy_args)
        )
