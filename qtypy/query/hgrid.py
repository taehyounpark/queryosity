
import numpy as np

from .definition import definition

class hgrid(definition):

    def __init__(self, hname : str, *, 
            dtype : str = 'float', 
            nx : int = 1, xmin : float = 0, xmax : float = 1,
            xbins  : np.array = None,
            grid_x : np.array = None, 
            grid_y : np.array = None, 
            grid_z : np.array = None, 
            n_toys : int = 0
            ):
        super().__init__()

        self.ndim = 1 + (grid_y is not None) + (grid_z is not None)
        self.hname = hname
        self.dtype = dtype
        self.n_toys = n_toys

        self.grid_x = grid_x
        self.grid_y = grid_y
        self.grid_z = grid_z

        self.nx = nx
        self.xmin = xmin
        self.xmax = xmax
        self.xbins = xbins

        self.xvec = f"std::vector<double>({{{', '.join([str(edge) for edge in grid_x])}}})"

        if self.ndim >= 2:
            self.yvec = f"std::vector<double>({{{', '.join([str(edge) for edge in grid_y])}}})"

        if self.ndim == 3:
            self.zvec = f"std::vector<double>({{{', '.join([str(edge) for edge in grid_z])}}})"

        if all([arg is None for arg in (xbins, xmin, xmax)]):
            self.xbins = f"std::vector<double>({{{', '.join([str(edge) for edge in xbins])}}})"
        else:
            self.xbins = f"{nx},{xmin},{xmax}"

    def __str__(self):
        filled_cols = getattr(self, 'filled_columns', [])

        # First parentheses from columns *after* ndim
        cols = ''.join(['(' + ', '.join(colset[self.ndim:]) + ')' for colset in filled_cols])

        # Histogram type string with optional Bootstrap
        toys = f", {self.n_toys} toys" if self.n_toys else ""
        result_type = f'{"TH1" if not self.n_toys else "TH1Bootstrap"}("{self.hname}", {self.xmin} < x < {self.xmax}{toys})'

        # Helper: build dimension string with skipped column names in parentheses
        def grid_str(index_char, grid_array, colset_idx):
            if grid_array is None:
                return ""
            # Get the corresponding column names that were skipped
            skipped_cols = []
            for colset in filled_cols:
                if len(colset) > colset_idx:
                    skipped_cols.append(colset[colset_idx])
            skipped_cols_str = ''.join([f'({c})' for c in skipped_cols])
            return f"[{index_char} = 0:{len(grid_array)-2} {skipped_cols_str}]"

        dim_indices = []
        if self.grid_x is not None:
            dim_indices.append(grid_str('i', self.grid_x, 0))
        if self.ndim > 1 and self.grid_y is not None:
            dim_indices.append(grid_str('j', self.grid_y, 1))
        if self.ndim > 2 and self.grid_z is not None:
            dim_indices.append(grid_str('k', self.grid_z, 2))

        return f'{cols} → {result_type}' + ''.join(dim_indices)

    @property
    def cpp_result_type(self):
        base_type = "TH1" 
        if self.n_toys > 0: base_type += "Bootstrap"

        # Map ndim to nested vector type
        if self.ndim == 1:
            return f"std::vector<std::shared_ptr<{base_type}>>"
        elif self.ndim == 2:
            return f"std::vector<std::vector<std::shared_ptr<{base_type}>>>"
        else:
            return f"std::vector<std::vector<std::vector<std::shared_ptr<{base_type}>>>>"

    @property
    def cpp_result_call(self):
        return 'result()'

    @property
    def py_result_wrapper(self):
        def result_wrapper(vec):
            if self.ndim == 1:
                return [vec[i] for i in range(len(self.grid_x)-1)]

            elif self.ndim == 2:
                hg = []
                for i in range(len(self.grid_x)-1):
                    row = [vec[i][j] for j in range(len(self.grid_y)-1)]
                    hg.append(row)
                return hg

            elif self.ndim == 3:
                hg = []
                for i in range(len(self.grid_x)-1):
                    plane = []
                    for j in range(len(self.grid_y)-1):
                        stack = [vec[i][j][k] for k in range(len(self.grid_z)-1)]
                        plane.append(stack)
                    hg.append(plane)
                return hg

        return result_wrapper

    @property
    def cpp_get_call(self):
        return (
            f'get(qty::query::output<qty::ROOT::'
            f'{"hgrid_with_toys" if self.n_toys > 0 else "hgrid"}'
            f'<{self.ndim},{self.dtype}>>'
            f'("{self.hname}"'
            f',{self.xvec}'
            f'{"," + self.yvec if self.ndim > 1 else ""}'
            f'{"," + self.zvec if self.ndim > 2 else ""}'
            f',{self.xbins}'
            f'{"," + str(self.n_toys) if self.n_toys > 0 else ""}'
            f'))'
        )
