
import numpy as np

from .definition import definition

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
        self.dtype = dtype
        self.n_toys = n_toys

        if xbins is None:
            self.xbins = f"{nx},{xmin},{xmax}"
            self.nx = nx
            self.xmin = xmin
            self.xmax = xmax
        else:
            if dtype == 'std::string':
                bin_items = ', '.join(f'"{b}"' for b in xbins)
                self.xbins = f"std::vector<std::string>{{{ {bin_items} }}}"
                self.nx = len(xbins)-1
                self.xmin = xbins[0]
                self.xmax = xbins[-1]
            else:
                self.xbins = f"std::vector<double>({{{', '.join([str(edge) for edge in xbins])}}})"
                self.nx = len(xbins)-1
                self.xmin = xbins[0]
                self.xmax = xbins[-1]

        if all([arg is None for arg in (ybins, ny, ymin, ymax)]):
            self.ybins = None
        elif ybins is None:
            self.ybins = f"{ny},{ymin},{ymax}"
            self.ny = ny
            self.ymin = ymin
            self.ymax = ymax
        elif all([arg is None for arg in (ny, ymin, ymax)]):
            if dtype == 'std::string':
                bin_items = ', '.join(f'"{b}"' for b in ybins)
                self.ybins = f"std::vector<std::string>{{{ {bin_items} }}}"
                self.ny = len(ybins)-1
                self.ymin = ybins[0]
                self.ymax = ybins[-1]
            else:
                self.ybins = f"std::vector<double>({{{', '.join([str(edge) for edge in ybins])}}})"
                self.ny = len(ybins)-1
                self.ymin = ybins[0]
                self.ymax = ybins[-1]
        else:
            raise ValueError("invalid y-axis configuration")

        if all([arg is None for arg in (zbins, nz, zmin, zmax)]):
            self.zbins = None
        elif zbins is None:
            self.zbins = f"{nz},{zmin},{zmax}"
            self.nz = nz
            self.zmin = zmin
            self.zmax = zmax
        elif all([arg is None for arg in (nz, zmin, zmax)]):
            self.zbins = f"std::vector<{self.dtype}>({{{', '.join([str(edge) for edge in zbins])}}})"
            self.nz = len(zbins)-1
            self.zmin = zbins[0]
            self.zmax = zbins[-1]
        else:
            raise ValueError("invalid z-axis configuration")

        self.ndim = 1 if self.ybins is None else 2 if self.zbins is None else 3

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
            f'{"," + self.ybins if self.ndim > 1 else ""}'
            f'{"," + str(self.n_toys) if self.n_toys > 0 else ""}'
            f'))'
        )
