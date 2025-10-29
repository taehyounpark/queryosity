import ROOT
from ..node import column

# -------------------------------------------------------------------
# jitted column class
# -------------------------------------------------------------------

class jitted(column):
    """
    Column defined by a JIT-compiled C++ function declared via ROOT.Numba.Declare.
    """
    def __init__(self, func_name: str):
        super().__init__()
        self.func_name = func_name
        self.args = []  # arguments passed via __call__

    def __call__(self, *args):
        """
        When called, store the argument names (string identifiers of columns).
        """
        self.args = list(args)
        return self  # so we can chain or assign directly

    def __str__(self):
        return f"{self.func_name}({', '.join([self.args])})"

    @property
    def cpp_initialization(self):
        # Only keep args that exist in df.columns
        column_args = [arg for arg in self.args if arg in self.df.columns]
        call_args = [f"{arg}.value()" for arg in column_args]
        lazy_args = [self.df.columns[col_name].cpp_identifier for col_name in self.args]

        # Generate lambda argument list with correct observable types
        lambda_args = [
            f"qty::column::observable<{self.df.columns[arg].cpp_value_type}> {arg}" 
            for arg in column_args
        ]

        # generate declarations
        val_decls = " ".join([f"auto {arg}_value = {arg}.value();" for arg in column_args])
        call_args = ", ".join([f"{arg}_value" for arg in column_args])

        lambda_wrapped = f"""[]( {', '.join(lambda_args)} ){{
            {val_decls}
            return Numba::{self.func_name}({call_args});
        }}"""


        # Return the full define-evaluate expression
        return f"{self.df.cpp_identifier}.define(qty::column::expression({lambda_wrapped})).evaluate({', '.join(lazy_args)})"


    @property
    def cpp_value_type(self):
        return f'qty::column::value_t<typename decltype({self.cpp_identifier})::action_type>'


# -------------------------------------------------------------------
# jit decorator
# -------------------------------------------------------------------

def jit(arg_types, return_type):
    """
    Decorator that registers a Python function with ROOT.Numba.Declare,
    and returns a callable jitted column factory.
    """
    def decorator(func):
        func_name = func.__name__

        # Register function with ROOT.Numba
        ROOT.Numba.Declare(arg_types, return_type)(func)

        # Return a callable column.jitted object (factory for columns)
        return jitted(func_name)

    return decorator