import cppyy

from rich.console import Console
from rich.live import Live
from rich.table import Table

from .cpputils import cpp_instantiable
from .query import lazyquery, lazyresult

class dataflow(cpp_instantiable):
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
        self._compiled = False

        self.multithreaded = multithreaded
        self.n_threads = 0 if not multithreaded else n_threads
        self.n_rows = n_rows

        self.name = "df"

        self.dataset = None

        self.columns = {}
        self.selections = {}

        self.bookkeepers = {}
        self.queries = {}
        self.results = {}

    @property
    def cpp_initialization(self):
        return f"""qty::dataflow(qty::multithread::enable({self.n_threads}),qty::dataset::head({self.n_rows}))"""

    def load(self, ds):
        ds.df = self
        self.dataset = ds
        return self

    def compute(self, definitions: dict):
        """
        Define additional columns in the dataflow.

        Parameters
        ----------
        definitions : dict
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
        table = Table(expand=True)
        table.add_column("Column")
        table.add_column("Definition")
        for column_name, column_node in definitions.items():
            table.add_row(f"{column_name}", f"{str(column_node)}")
            column_node.name = column_name
            column_node.df = self
        console = Console()
        console.print(table)
        self.columns.update(definitions)
        return self

    def apply(self, selections: dict):
        table = Table(expand=True)
        table.add_column("Preselection")
        table.add_column("Selection")
        table.add_column("Expression")
        for selection_name, selection_node in selections.items():
            table.add_row(f"{selection_node.preselection_name}", f"{selection_name}", f"{str(selection_node)}")
            selection_node.name = selection_name
            selection_node.df = self
        console = Console()
        console.print(table)
        self.selections.update(selections)
        return self

    def get(self, bookkeepers: dict):
        self._compiled = False
        lazy_results = {}
        table = Table(expand=True)
        table.add_column("Selection")
        table.add_column("Query")
        table.add_column("Definition")
        for query_name, bookkeeper in bookkeepers.items():
            lazy_results[query_name] = {}
            for selection_name in bookkeeper.booked_selections:
                query_node = lazyquery(self, bookkeeper, self.selections[selection_name])
                query_node.name = f"{query_name}_at_{selection_name}"
                lazy_results[query_name][selection_name] = lazyresult(query_node)
                table.add_row(f"{selection_name}", f"{query_name}", f"{query_node.bkpr}")
        console = Console()
        console.print(table)
        return lazy_results

    def compile(self):

        if self._compiled: return
        self._compiled = True

        self.instantiate()

        self.dataset.instantiate()

        for column_name, column_node in self.columns.items():
            column_node.instantiate()

        # current selection is the global dataflow
        self.current_selection = self
        for selection_name, selection_node in self.selections.items():
            selection_node.instantiate()
            # now the selection is at the latest applied
            self.current_selection = selection_node

        # # queries
        # table = Table(expand=True)
        # table.add_column("Selection")
        # table.add_column("Query")
        # table.add_column("Definition")
        # with Live(table, auto_refresh=False, vertical_overflow="visible") as display:
        #     for query_name, booked_selections in self.queries.items():
        #         for selection_name, query_node in booked_selections.items():
        #             query_node.instantiate()
        #             result_node = lazyresult(query_node)
        #             self.results[selection_name][query_name] = result_node
        #             table.add_row(f"{selection_name}", f"{query_name}", f"{query_node.bookkeeper}")
        #             display.refresh()
