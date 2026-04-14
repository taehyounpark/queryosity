
import numpy as np

from .definition import definition

class prof(definition):

    def __init__(self, hname : str, *, 
            dtype : str = 'float', 
            xbins : np.array = None, 
            ybins : np.array = None
            ):
        super().__init__()

        self.hname = hname
        self.dtype = dtype

        self.xbins = f"std::vector<double>({{{', '.join([str(edge) for edge in xbins])}}})"
        self.nx = len(xbins)-1
        self.xmin = xbins[0]
        self.xmax = xbins[-1]

        if ybins is not None:
            try:
                self.ybins = f"std::vector<double>({{{', '.join([str(edge) for edge in ybins])}}})"
            except:
                raise ValueError("invalid y-axis configuration")
            self.ny = len(ybins)-1
            self.ymin = ybins[0]
            self.ymax = ybins[-1]

        self.ndim = 1 if self.ybins is None else 2

    def __str__(self):
        xax = f', {self.xmin} < x < {self.xmax}'
        yax = f', {self.ymin} < y < {self.ymax}' if self.ndim > 1 else ''
        cols = ''.join(['('+', '.join(colset)+')' for colset in self.filled_columns])
        return f'{cols} → {self.cpp_result_type[:-1]}("{self.hname}"{xax}{yax})'

    @property
    def cpp_result_type(self):
        return "TProfile*" if self.ndim == 1 else "TProfile2D*"

    @property
    def cpp_result_call(self):
        # std::shared_ptr::get()
        return 'result().get()'

    @property
    def cpp_get_call(self):
        return (
            f'get(qty::query::output<qty::ROOT::'
            f'prof<{self.ndim},{self.dtype}>>'
            f'("{self.hname}"'
            f',{self.xbins}'
            f'{"," + self.ybins if self.ndim > 1 else ""}'
            f'))'
        )
