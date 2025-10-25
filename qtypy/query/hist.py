
import numpy as np

from . import definition

class hist(definition):

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
        return f'{cols} → {self.cpp_result_type[:-1]}("{self.hname}"{xax}{yax}{toys})'

    @property
    def cpp_result_type(self):
        return "TH{}{}*".format(
            self.ndim,
            'Bootstrap' if self.n_toys > 0 else ''
        )

    @property
    def cpp_result_call(self):
        # std::shared_ptr::get()
        return 'result().get()'

    @property
    def cpp_get_call(self):
        return (
            f'get(qty::query::output<qty::ROOT::'
            f'{"hist_with_toys" if self.n_toys > 0 else "hist"}'
            f'<{self.ndim},{self.dtype}>>'
            f'("{self.hname}"'
            f',{self.xbins}'
            f'{"," + str(self.n_toys) if self.n_toys > 0 else ""}'
            f'))'
        )
