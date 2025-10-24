import cppyy

from .. import lazynode

class constant(lazynode):
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

    @property
    def cpp_initialization(self):
        return """{df_id}.define(qty::column::constant<{value_type}>({value}));""".format(
            cpp_id=self.cpp_identifier,
            df_id=self.df.cpp_identifier,
            value_type=self.value_type,
            value=self.value
        )