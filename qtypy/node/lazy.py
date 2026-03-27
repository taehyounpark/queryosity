from ..cpp import cpp_binding

from abc import abstractmethod

class lazy(cpp_binding):

    def __init__(self):
        super().__init__()
        self._df = None

    @property
    def df(self):
        if self._df is None:
            raise RuntimeError("lazy node not belong to any dataflow")
        return self._df

    @df.setter
    def df(self, value):
        if self._df is not None:
            raise RuntimeError("lazy node already assigned to a dataflow")
        self._df = value

    def _instantiate(self):
        if not self._instantiated:
            self._instantiated = True
            cpp_line = '''{type} {id} = {init};'''.format(
                type = self.cpp_type,
                id = self.cpp_identifier,
                init = self.cpp_initialization
            )
            self.df.cpp_lines.append(cpp_line)