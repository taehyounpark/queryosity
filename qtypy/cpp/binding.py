import cppyy

from abc import ABC, abstractmethod
from functools import cached_property
from itertools import count

_instance_counter = count()

class cpp_binding(ABC):
    def __init__(self):
        self._instance_index = next(_instance_counter)
        self._instantiated = False

        self.cpp_prefix = '__qtypy__'
        self.name = None

    @property
    def cpp_type(self) -> str:
        return 'auto'

    @property
    @abstractmethod
    def cpp_initialization(self) -> str:
        pass

    @cached_property
    def cpp_identifier(self) -> str:
        identifier = f'{self.cpp_prefix}{self.name}_{self._instance_index}'
        return identifier

    @cached_property
    def cpp_instance(self):
        self.instantiate()
        return getattr(cppyy.gbl, self.cpp_identifier, None)

    def instantiate(self):
        if not self._instantiated:
            self._instantiated = True
            return cppyy.cppdef('''{type} {id} = {init};'''.format(
                type = self.cpp_type,
                id = self.cpp_identifier,
                init = self.cpp_initialization
            ))