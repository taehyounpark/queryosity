import cppyy

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
    multithreaded : bool, optional
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
    >>> df = dataflow(multithreaded=True)  # use all available threads
    >>> df = dataflow(multithreaded=True, n_threads=64)  # use up to 64 threads
    """

    def __init__(self, *, multithreaded : bool = False, n_threads: int = -1, n_rows: int = -1):
        super().__init__()

        self.multithreaded = multithreaded
        self.n_threads = 0 if not multithreaded else n_threads
        self.n_rows = n_rows

        self.dataset = None

        self.columns = {}
        self.selections = {}
        self.current_selection = self

        self.instantiate()

    @property
    def cpp_initialization(self):
        return f"""qty::dataflow(qty::multithread::enable({self.n_threads}),qty::dataset::head({self.n_rows}))"""

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
        it = iter(selections.items())

        # First selection
        first_name, first_node = next(it)
        first_node.contextualize(self, first_name)
        first_node.instantiate()
        df_at_sel = self.at(first_name)

        # Remaining selections
        for sel_name, sel_node in it:
            sel_node.contextualize(self, sel_name)
            sel_node.instantiate()
            df_at_sel = self.at(sel_name)

        return df_at_sel

    def at(self, selection: str):
        return dataflow_at_selection(self, selection)

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

class dataflow_at_selection:

    def __init__(self, df, sel_name):
        self.df = df
        self.sel_name = sel_name
        if sel_name not in self.df.selections:
            raise KeyError(f"selection '{sel_name}' not defined in dataflow")

    def compute(self, columns: dict):
        raise RuntimeError(
            "compute columns from the main dataflow"
        )

    def select(self, selections: dict):
        """Apply further selections starting from this branch"""
        self.df.current_selection = self.df.selections[self.sel_name]
        return self.df.select(selections)

    def output(self, query_defn):
        """Book query at this selection"""
        self.df.current_selection = self.df.selections[self.sel_name]
        return self.df.output(query_defn)

    # DSL syntax
    __or__ = compute     # not supported
    __truediv__ = select # sel / {...}
    __rshift__ = output  # sel >> query
