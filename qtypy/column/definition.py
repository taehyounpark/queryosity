import cppyy

from ..cpp import cpp_binding

from functools import cached_property

class definition(cpp_binding):
    """
    Column defined by a one-line JIT-compiled C++ expression.

    Parameters
    ----------
    expr : str
        A string containing a C++ expression. Identifiers within the expression
        will be parsed to determine the required arguments.
    """
    def __init__(self, defn: str):
        super().__init__()
        self.defn = defn

    def __str__(self):
        return self.defn

    def __call__(self, *args):
        self.args = args

    def instantiate(self, df):
        # only keep args that exist in df.columns
        column_args = [arg for arg in self.args if arg in df.columns]
        lmbd_args = [f'{df.columns[arg].value_type} const& {arg}' for arg in column_args]
        lmbd_defn = '[](' + ', '.join(lmbd_args) + '){return (' + self.expr + ');}'
        lazy_args = [df.columns[arg].cpp_identifier for arg in column_args]

        cppyy.cppdef("""auto {cpp_id} = {df_id}.define(qty::column::expression({expr})).evaluate({args});""".format(
            cpp_id=self.cpp_identifier,
            df_id=df.cpp_identifier,
            expr=lmbd_defn,
            args=', '.join(lazy_args)
        ))

        self.value_type = f'qty::column::value_t<typename decltype({self.cpp_identifier})::action_type>'