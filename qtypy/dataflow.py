from rich.console import Console
from rich.live import Live
from rich.table import Table

from .cpp import cpp_binding
from .node import query, result

class dataflow(cpp_binding):
    """
    qtypy layer for ``qty::dataflow``.

    Provides a high-level interface for defining, selecting, and querying columns
    from datasets, with optional multi-threaded execution.

    Parameters
    ----------
    multithread : bool, optional
        Enable multithreading (default is False).
    n_threads : int, optional
        Number of threads to use for processing (default is -1, meaning all available threads).
    n_rows : int, optional
        Number of rows to process from the dataset (default is -1, meaning all rows).

    Attributes
    ----------
    dataset : object or None
        Dataset attached to the dataflow. Initially `None`.
    columns : dict
        Mapping of column names to column definitions.
    selections : dict
        Mapping of selection names to selection objects.

    Notes
    -----
    The `results` attribute is computed lazily, meaning that queries are only executed
    when their results are actually retrieved. This can improve performance for large datasets
    or expensive computations.

    Examples
    --------
    >>> df = dataflow()                 # use all available threads
    >>> df = dataflow(n_threads=8)      # use (up to) 8 threads
    >>> df = dataflow(enable_mt=False)  # single-threaded
    """

    def __init__(self, *, n_threads: int = -1, n_rows: int = -1, enable_mt : bool = True):
        super().__init__()

        self.enable_mt = enable_mt
        self.n_threads = n_threads
        self.n_rows = n_rows

        self.dataset = None

        self.columns = {}
        self.selections = {}
        self.current_selection = self

        self.instantiate()

    @property
    def cpp_initialization(self):
        return f"""qty::dataflow(qty::multithread::enable({self.enable_mt * self.n_threads}),qty::dataset::head({self.n_rows}))"""

    def input(self, ds):
        ds.contextualize(self)
        return self

    def compute(self, columns: dict):
        """
        Define additional columns in the dataflow.

        Parameters
        ----------
        columns : dict
            A dictionary mapping column names (strings) to one of the following:
            
        - ``qtypy.dataset.column``  
            Existing quantity in dataset.

        - ``qtypy.column.constant``  
            Constant value of any C++ data type.

        - ``qtypy.column.expression``  
            JIT-compiled one-line C++ expression.

        - ``qtypy.column.definition``  
            Compiled C++ implementation of
            ``qty::column::definition<Ret(Args...)>``.

        Returns
        -------
        self
            Enables method chaining.
        """
        for column_name, column_node in columns.items():
            column_node.contextualize(self, column_name)
            column_node.instantiate()
        return self

    def select(self, selections: dict):
        """Define selections in order and return a branch at the last one."""

        for sel_name, sel_node in selections.items():
            sel_node.contextualize(self, sel_name)
            sel_node.instantiate()

        return self

    def at(self, selection: str):
        if selection not in self.selections:
            raise KeyError(f"selection '{selection}' not found in dataflow.")
        self.current_selection = self.selections[selection]
        return self

    def output(self, query_defn):
        # issue new lazy<query> node everytime so existing definitions can be recycled later
        query_node = query(query_defn)
        query_node.contextualize(self)
        # return the (not yet instantiated) result node
        return result(query_node)

    # DSL syntax
    __lshift__ = input
    __or__ = compute
    __matmul__ = at
    __truediv__ = select
    __rshift__ = output