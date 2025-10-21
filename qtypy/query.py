import cppyy
import numpy as np

from abc import ABC, abstractmethod
from dataclasses import dataclass, field

from .cpp import cpp_binding

@dataclass
class report:
    """Keep track of results @ selections"""
    hists: dict = field(default_factory=dict)
    trees: dict = field(default_factory=dict)

class lazy(cpp_binding):

    def __init__(self, bookkeeper, selection):
        super().__init__()
        self.bookkeeper = bookkeeper
        self.selection = selection

    def instantiate(self, df):
        fill_calls = []
        for column_names in self.bookkeeper.filled_columns:
            fill_call = 'fill('
            fill_call += ', '.join([f'{df.columns[column_name].cpp_identifier}' for column_name in column_names])
            fill_call += ')'
            fill_calls.append(fill_call)
        at_call = f'at({self.selection.cpp_identifier})'
        cppyy.cppdef(f'auto {self.cpp_identifier} = {df.cpp_identifier}.{self.bookkeeper.get_call}.{".".join(fill_calls)}.{at_call};')

class result(cpp_binding):
    def __init__(self, lazy):
        super().__init__()
        self.lazy = lazy
        self.name = f'result_{lazy.name}'

    def instantiate(self):
        cppyy.cppdef(f'auto {self.cpp_identifier} = {self.lazy.cpp_identifier}.{self.lazy.bookkeeper.result_call};')

    def get(self):
        return self.cpp_instance

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
        """C++ typename of ``lazy::result()``"""
        pass

    @property
    @abstractmethod
    def result_call(self):
        """C++ code string to retrieve ``lazy::result()``"""
        pass

    @property
    @abstractmethod
    def get_call(self):
        """C++ code string call ``dataflow::get()``"""
        pass

class hist(bookkeeper):

    def __init__(self, hname : str, *, 
            dtype : str = 'float', 
            nx : int = None, xmin : float = None, xmax : float = None,
            ny : int = None, ymin : float = None, ymax : float = None,
            nz : int = None, zmin : float = None, zmax : float = None,
            xbins : np.array = None, 
            ybins : np.array = None, 
            zbins : np.array = None,
            n_toys : int = 0
            ):
        super().__init__()

        self.hname = hname
        self.ndim = 1 if ybins is None and zbins is None else 2 if zbins is None else 3
        self.dtype = dtype

        if xbins is None:
            self.xbins = f"{nx},{xmin},{xmax}"
            self.nx = nx
            self.xmin = xmin
            self.xmax = xmax
        else:
            self.xbins = f"std::vector<{self.dtype}>({{{', '.join([str(edge) for edge in xbins])}}})"
            self.nx = len(xbins)-1
            self.xmin = xbins[0]
            self.xmax = xbins[-1]

        self.n_toys = n_toys

    def __str__(self):
        # xax = 
        xax = f', {self.xmin} < x < {self.xmax}'
        yax = f', {self.ybins}' if self.ndim > 1 else ''
        toys = f', {self.n_toys} toys' if self.n_toys else ''
        cols = ''.join(['('+', '.join(colset)+')' for colset in self.filled_columns])
        return f'{cols} -> {self.result_type}("{self.hname}"{xax}{yax}{toys})'

    @property
    def result_type(self):
        return "TH{}{}".format(
            self.ndim,
            'Bootstrap' if self.n_toys > 0 else ''
        )

    @property
    def result_call(self):
        # std::shared_ptr::get()
        return 'result().get()'

    @property
    def get_call(self):
        return (
            f'get(qty::query::output<qty::ROOT::'
            f'{"hist_with_toys" if self.n_toys > 0 else "hist"}'
            f'<{self.ndim},{self.dtype}>>'
            f'("{self.hname}"'
            f',{self.xbins}'
            f'{"," + str(self.n_toys) if self.n_toys > 0 else ""}'
            f'))'
        )

