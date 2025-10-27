from ..cpp import cpp_binding

from abc import abstractmethod

class lazy(cpp_binding):

    def __init__(self):
       super().__init__() 
       self.cpp_prefix += 'lazy_'
       self._df = None

    @property
    def df(self):
        if self._df is None:
            raise RuntimeError("lazy node not belong to any dataflow graph")
        return self._df

    @df.setter
    def df(self, value):
        if self._df is not None:
            raise RuntimeError("lazy node already assigned to a dataflow graph")
        self._df = value

    @abstractmethod
    def contextualize(self, df, name):
        pass
