import cppyy

from .cpp import cpp_binding 
from .cpp import parse_cpp_identifiers

from functools import cached_property

class constant(cpp_binding):
    """
    Column representing a constant value.

    Parameters
    ----------
    value : any
        The constant value to store in the column.
    value_type : str, optional
        The C++ type name for the value.
    """
    def __init__(self, value, value_type = None):
        super().__init__()
        self.value = value
        if value_type is None:
            self.value_type = type(value).__name__

        self.cpp_prefix = '_df_column'

    def instantiate(self, df):
        return cppyy.cppdef("""auto {cpp_id} = {df_id}.define(qty::column::constant<{value_type}>({value}));""".format(
            cpp_id=self.cpp_identifier,
            df_id=df.cpp_identifier,
            value_type=self.value_type,
            value=self.value
        ))

class expression(cpp_binding):
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
        self.args = parse_cpp_identifiers(expr)

        self.cpp_prefix = '_df_column'

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

class definition(cpp_binding):
    """
    Column defined by a compiled C++ class implementing a quantity column.

    Parameters
    ----------
    defn : str
        The name of the C++ class implementing
        ``qty::column::definition<Ret(Args...)>``.
    """
    def __init__(self, defn: str):
        super().__init__()
        self.defn = defn
        self.cpp_prefix = '_df_column'

    def __call__(self, *columns):
        self.args = columns

    