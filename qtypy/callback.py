import cppyy
import numpy as np

from abc import ABC, abstractmethod
from dataclasses import dataclass, field

from .cpp import cpp_binding

@dataclass
class bookkeeper(ABC):
    filled_columns: list = field(default_factory=list)
    booked_selections: list = field(default_factory=list)

    def fill(self, *columns):
        # TODO: support more than one fill() calls
        self.filled_columns.append(columns)
        return self

    def at(self, *selections):
        self.booked_selections.extend(selections)
        return self

    @property
    @abstractmethod
    def result_type(self):
        """C++ typename of lazy::result()"""
        pass

    @property
    @abstractmethod
    def result_call(self):
        """C++ code string to retrieve lazy::result()"""
        pass

    @property
    @abstractmethod
    def get_call(self):
        """C++ code string call dataflow::get()"""
        pass

class toys(bookkeeper):

    def __init__(self, n_toys : int = 100):
        self.n_toys = n_toys

    @property
    def result_type(self):
        return "TH{}{}".format(
            self.ndim,
            'Bootstrap' if self._bootstrapped else ''
        )

    @property
    def result_call(self):
        # std::shared_ptr::get()
        return 'result().get()'

    @property
    def get_call(self):
        return (
            f'get(qty::query::output<qty::ROOT::Hist<{self.ndim},{self.dtype}>>'
            f'("{self.hname}",{self.xbinning}))'
        ) 