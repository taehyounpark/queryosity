from abc import ABC, abstractmethod

from ..node import query, result

class definition(ABC):

    def __init__(self):
        self.filled_columns : list[list[str]] = []
        pass

    def fill(self, *columns):
        self.filled_columns.append(columns)
        return self

    @property
    def py_result_wrapper(self):
        return lambda x: x

    @property
    @abstractmethod
    def cpp_get_call(self):
        """C++ code string to call ``dataflow::get()``"""
        pass

    @property
    def cpp_result_type(self):
        return 'auto'

    @property
    def cpp_result_call(self):
        return 'result()'

    def output(self, df):
        # issue new lazy<query> node everytime so existing definitions can be recycled later
        query_node = query(self)
        query_node._contextualize(df)
        # return the (not yet instantiated) result node
        return result(query_node)