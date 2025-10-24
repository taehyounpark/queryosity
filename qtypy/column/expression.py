import cppyy

from ..cpputils import parse_cpp_expression
from .. import lazynode

from functools import cached_property

class expression(lazynode):
    """
    Column defined by a one-line JIT-compiled C++ expression.

    Parameters
    ----------
    expr : str
        A string containing a C++ expression. Identifiers within the expression
        will be parsed to determine the required arguments.
    """
    def __init__(self, expr: str):
        super().__init__()
        self.expr = expr
        self.args = parse_cpp_expression(expr)

    def __str__(self):
        return self.expr

    @property
    def cpp_initialization(self):
        # only keep args that exist in columns
        column_args = [arg for arg in self.args if arg in self.df.columns]
        lmbd_args = [f'{self.df.columns[arg].value_type} const& {arg}' for arg in column_args]
        lmbd_defn = '[](' + ', '.join(lmbd_args) + '){return (' + self.expr + ');}'
        lazy_args = [self.df.columns[arg].cpp_identifier for arg in column_args]

        self.value_type = f'qty::column::value_t<typename decltype({self.cpp_identifier})::action_type>'

        return """{df_id}.define(qty::column::expression({expr})).evaluate({args});""".format(
            cpp_id=self.cpp_identifier,
            df_id=self.df.cpp_identifier,
            expr=lmbd_defn,
            args=', '.join(lazy_args)
        )
