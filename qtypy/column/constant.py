from ..node import column

class constant(column):
    """
    Column representing a constant value.

    Parameters
    ----------
    value : any
        The constant value to store in the column.
    cpp_value_type : str, optional
        The C++ type name for the value.
    """
    def __init__(self, value, dtype = None):
        super().__init__()
        self.value = value
        self.dtype = dtype

    @property
    def cpp_value_type(self):
        if self.dtype is None:
            return type(self.value).__name__
        return self.dtype

    @property
    def cpp_initialization(self):
        return """{df_id}.define(qty::column::constant<{cpp_value_type}>({value}));""".format(
            cpp_id=self.cpp_identifier,
            df_id=self.df.cpp_identifier,
            cpp_value_type=self.cpp_value_type,
            value=self.value
        )