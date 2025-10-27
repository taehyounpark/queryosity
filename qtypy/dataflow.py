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

        self.name = "df"

        self.dataset = None

        self.columns = {}
        self.selections = {}
        self.current_selection = self

        self.instantiate()

    @property
    def cpp_initialization(self):
        return f"""qty::dataflow(qty::multithread::enable({self.n_threads}),qty::dataset::head({self.n_rows}))"""

    def __lshift__(self, ds):
        """Pipeline operator for input dataset: df << {'events' : dataset.tree(...)}"""
        return self.input(ds)

    def __or__(self, columns: dict):
        """Pipeline operator for column definitions: df | {'col': column(...) << 'events' }"""
        return self.compute(columns)

    def __matmul__(self, selection: str):
        return self.at(selection)

    def __rshift__(self, query):
        return self.output(query)

    def input(self, ds):
        ds.contextualize(self)
        ds.instantiate()
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

        - ``qtypy.selection.filter``  
            Cut expression

        - ``qtypy.selection.weight``  
            Weight expression

        Returns
        -------
        self
            Enables method chaining.
        """
        for column_name, column_node in columns.items():
            column_node.contextualize(self, column_name)
            column_node.instantiate()
        return self

    def at(self, selection: str):
        return dataflow_at_selection(self, selection)

    def output(self, query_defn):
        # issue new lazy<query> node everytime so existing definitions can be recycled later
        query_node = query(query_defn)
        query_node.contextualize(self)
        query_node.instantiate()
        # return the (not yet instantiated) result node
        return result(query_node)

class dataflow_at_selection:

    def __init__(self, df, sel_name):
        self.df = df
        self.sel_name = sel_name

    def dataflow_has_selection(self) -> bool:
        return (self.sel_name in self.df.selections)

    def compute(self, selections: dict):
        if not self.dataflow_has_selection():
            raise KeyError("selection not defined in dataflow (yet?)")
        # start from specified selection
        self.df.current_selection = self.df.selections[self.sel_name]
        return self.df.compute(selections)

    def output(self, query_defn):
        if not self.dataflow_has_selection():
            raise KeyError("selection not defined in dataflow (yet?)")
        # book at specified selection
        self.df.current_selection = self.df.selections[self.sel_name]  
        return self.df.output(query_defn)

    def __rshift__(self, query):
        return self.output(query)

    def __or__(self, selection: str):
        return self.compute(selection)