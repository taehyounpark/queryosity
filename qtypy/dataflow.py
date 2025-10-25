import cppyy

from rich.console import Console
from rich.live import Live
from rich.table import Table

from .cpp import cpp_binding
from .query import query, result

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
    queries : dict
        Mapping of query names to query objects.
    results : dict
        Lazily evaluated results of queries. Computation occurs only upon property access.

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

        self.name = "df"

        self.datasets = {}
        self.current_dataet = None

        self.columns = {}
        self.selections = {}
        self.current_selection = self

        self.bookkeepers = {}
        self.queries = {}
        self.results = {}

        self.instantiate()

    @property
    def cpp_initialization(self):
        return f"""qty::dataflow(qty::multithread::enable({self.n_threads}),qty::dataset::head({self.n_rows}))"""

    def __lshift__(self, ds):
        """Pipeline operator for input dataset: df << {'events' : dataset.tree(...)}"""
        return self.load(ds)

    def __or__(self, columns: dict):
        """Pipeline operator for column definitions: df | {'col': column(...) << 'events' }"""
        return self.compute(columns)

    def __matmul__(self, selections: dict):
        """Pipeline operator for selections: df @ {'sel': filter(...) @ 'presel' }"""
        return self.select(selections)

    def __rshift__(self, query):
        """Pipeline operator for queries: df >> {'hist': hist(...) @ ['sel_a', 'sel_b']}"""
        return self.get(query)

    def load(self, datasets):
        for ds_name, ds in datasets.items():
            ds.df = self
            ds.instantiate()
            self.datasets[ds_name] = ds
            self.current_dataset = ds
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
            column_node.name = column_name
            column_node.df = self
            column_node.instantiate()
        self.columns.update(columns)
        return self

    def select(self, selections: dict):
        for selection_name, selection_node in selections.items():
            selection_node.name = selection_name
            selection_node.df = self
            selection_node.instantiate()
            # self.columns[selection_name] = selection_node
            self.selections[selection_name] = selection_node
            self.current_selection = selection_node
        return self

    def get(self, query_node):
        query_node.df = self
        query_node.instantiate()
        return result(query_node)