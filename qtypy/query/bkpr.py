from abc import ABC, abstractmethod
from dataclasses import dataclass, field

@dataclass
class bkpr(ABC):
    filled_columns: list = field(default_factory=list)
    booked_selections: list = field(default_factory=list)

    def fill(self, *columns):
        self.filled_columns.append(columns)
        return self

    def at(self, *selections):
        self.booked_selections.extend(selections)
        return self

    def __matmul__(self, selections):
        return self.at(*selections)

    @property
    @abstractmethod
    def result_type(self):
        """C++ typename of ``lazy::result()``"""
        pass

    @property
    def result_call(self):
        return 'result()'

    @property
    @abstractmethod
    def get_call(self):
        """C++ code string call ``dataflow::get()``"""
        pass