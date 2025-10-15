import cppyy

from .cpp import cpp_binding
from .query import lazy, result

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

        self.dataset = None

        self.columns = {}
        self.selections = {}
        self.queries = {}

        self.results = {}

    def instantiate(self):
        return cppyy.cppdef(
            f"""auto {self.cpp_identifier} = qty::dataflow(qty::multithread::enable({self.n_threads}),qty::dataset::head({self.n_rows}));"""
        )

    def load(self, ds):
        self.dataset = ds
        return self

    def read(self, columns: dict):
        """
        Read dataset columns into the dataflow.

        Parameters
        ----------
        columns : dict
            A dictionary mapping column names (strings) to their
            ``qtypy.dataset.column`` specifications.

        Returns
        -------
        self
            Enables method chaining.
        """
        for column_name, column_node in columns.items():
            column_node.name = column_name
        self.columns.update(columns)
        return self

    def define(self, definitions: dict):
        """
        Define additional columns in the dataflow.

        Parameters
        ----------
        definitions : dict
            A dictionary mapping column names (strings) to one of the following:
            

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
        for column_name, column_node in definitions.items():
            column_node.name = column_name
        self.columns.update(definitions)
        return self


    def apply(self, selections: dict):
        for selection_name, selection_node in selections.items():
            selection_node.name = selection_name
            # prepare for potential results at this selection
            self.results[selection_name] = {}
        self.selections.update(selections)
        return self

    def get(self, bookkeepers: dict):
        for query_name, bookkeeper in bookkeepers.items():
            self.queries[query_name] = {}
            for selection_name in bookkeeper.booked_selections:
                query_node = lazy(bookkeeper, self.selections[selection_name])
                query_node.name = f"{query_name}_at_{selection_name}"
                self.queries[query_name][selection_name] = query_node
        return self

    def analyze(self):

        self.instantiate()

        self.dataset.instantiate(self)

        print("=========="*8)
        print("Column")
        print('----------'*8)
        for column_name, column_node in self.columns.items():
            print(f'{column_name} = {str(column_node)}')
            column_node.instantiate(self)

        # current "selection" is the global dataflow
        print("=========="*8)
        print("Selection")
        print('----------'*8)
        self.current_selection = self
        for selection_name, selection_node in self.selections.items():
            selection_node.instantiate(self)
            print(f'{selection_name} = {str(selection_node)}')
            # now the selection is sitting at the last applied
            self.current_selection = selection_node

        # queries
        print("=========="*8)
        print("Query")
        print('----------'*8)
        for query_name, booked_selections in self.queries.items():
            for selection_name, query_node in booked_selections.items():
                query_node.instantiate(self)
                result_node = result(query_node)
                self.results[selection_name][query_name] = result_node
            print(f'{query_name} = {query_node} | {list(booked_selections.keys())}')
        print("=========="*8)

        # results
        for selection_name, results_at_selection in self.results.items():
            for query_name, result_node in results_at_selection.items():
                result_node.instantiate()
                self.results[selection_name][query_name] = result_node.get()

        return self.results
