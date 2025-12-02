from ..cpp import find_cpp_identifiers_and_replace_with_values
from ..node import column

class expression(column):
    """
    Column defined by a one-line JIT-compiled C++ expression.

    Parameters
    ----------
    expr : str
        A string containing a C++ expression. Identifiers within the expression
        will be parsed to determine the required arguments.
    """

    def __init__(self, expr: str, dtype: str = "auto"):
        super().__init__()
        self.expr = expr
        self.dtype = dtype

    def __str__(self):
        return self.expr

    @property
    def cpp_value_type(self) -> str:
        if self.dtype == "auto":
            return super().cpp_value_type
        return self.dtype

    @property
    def cpp_initialization(self):
        # only keep args that exist in columns
        column_args, expr = find_cpp_identifiers_and_replace_with_values(
            self.expr, set(self.df.columns)
        )
        lmbd_args = [
            f"qty::column::observable<{self.df.columns[arg].cpp_value_type}> {arg}"
            for arg in column_args
        ]

        lmbd_defn = (
            "[](" + ", ".join(lmbd_args) + f") -> {self.dtype} {{return {expr};}}"
        )
        lazy_args = [self.df.columns[arg].cpp_identifier for arg in column_args]

        return """{df_id}.define(qty::column::expression({expr})).evaluate({args})""".format(
            cpp_id=self.cpp_identifier,
            df_id=self.df.cpp_identifier,
            expr=lmbd_defn,
            args=", ".join(lazy_args),
        )

    @property
    def cpp_value_type(self):
        return f"qty::column::value_t<typename decltype({self.cpp_identifier})::action_type>"
