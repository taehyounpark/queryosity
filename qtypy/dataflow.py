from .cpp import cpp_binding
from . import column
from . import selection
from .node import query, result

# ==========================================
# Restricted DataFrame Views (Runtime Safety)
# ==========================================

class _dataflow_analysis:
    """
    Analysis context:
    - Mutation allowed
    - Queries (>>) forbidden
    """

    def __init__(self, df):
        self._df = df

    # Forward normal attribute access
    def __getattr__(self, name):
        return getattr(self._df, name)

    def __setitem__(self, key, value):
        self._df[key] = value

    def __and__(self, cut):
        return self._df & cut

    def __mul__(self, wgt):
        return self._df & wgt

    def __matmul__(self, sel):
        return self._df @ sel

    # Forbid queries inside analysis
    def __rrshift__(self, other):
        raise RuntimeError(
            "Query operator (>>) is not allowed inside @dataflow.analysis blocks."
        )


class _dataflow_output:
    """
    Query context:
    - Mutation forbidden
    - Queries allowed
    """

    def __init__(self, df):
        self._df = df

    def __getattr__(self, name):
        return getattr(self._df, name)

    # Allow read
    def __getitem__(self, key):
        return self._df[key]

    # Forbid mutation
    def __setitem__(self, key, value):
        raise RuntimeError(
            "Mutation (df[...] = ...) is not allowed inside @dataflow.query blocks."
        )

    # If you overload &, *, etc. for mutation, block them:
    def __and__(self, other):
        raise RuntimeError("Mutation via & is not allowed inside @dataflow.query.")

    def __mul__(self, other):
        raise RuntimeError("Mutation via * is not allowed inside @dataflow.query.")


# ==========================================
# Base Configured Step
# ==========================================

class _configured_step:
    def __init__(self, func, args, kwargs):
        self.func = func
        self.args = args
        self.kwargs = kwargs


# ==========================================
# Configured Analysis
# ==========================================

class _custom_analysis(_configured_step):
    def __ror__(self, df):
        safe_df = _dataflow_analysis(df)
        self.func(safe_df, *self.args, **self.kwargs)
        return df

# ==========================================
# Configured Query
# ==========================================

class _configured_output(_configured_step):
    def __rrshift__(self, df):
        safe_df = _dataflow_output(df)
        return self.func(safe_df, *self.args, **self.kwargs)

# ==========================================
# Definition Objects (Decorators)
# ==========================================

class _analysis:
    def __init__(self, func):
        self.func = func

    def __call__(self, *args, **kwargs):
        return _custom_analysis(self.func, args, kwargs)

    def __ror__(self, df):
        # allow df | analysis without args
        safe_df = _dataflow_analysis(df)
        self.func(safe_df)
        return df

class _output:
    def __init__(self, func):
        self.func = func

    def __call__(self, *args, **kwargs):
        return _configured_output(self.func, args, kwargs)

    def __rrshift__(self, df):
        safe_df = _dataflow_output(df)
        return self.func(safe_df)

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
    dataset : object
        Dataset attached to the dataflow. Initially `None`.
    columns : dict
        Mapping of column names to column definitions.
    selections : dict
        Mapping of selection names to selection objects.

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

        self._instantiate()

    @property
    def cpp_initialization(self):
        return f"""qty::dataflow(qty::multithread::enable({self.enable_mt * self.n_threads}),qty::dataset::head({self.n_rows}))"""

    def input(self, ds):
        ds._contextualize(self)
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
            column_node._contextualize(self, column_name)
            column_node._instantiate()
        return self

    def filter(self, cuts: dict):
        last_selection = None

        for cut_name, cut_expr in cuts.items():
            cut_node = selection.cut(cut_expr)
            cut_node._contextualize(self, cut_name)
            cut_node._instantiate()
            last_selection = cut_name

        return _dataflow_at_selection(self, last_selection)

    def weight(self, weights: dict):
        last_selection = None

        for wgt_name, wgt_expr in weights.items():
            wgt_node = selection.weight(wgt_expr)
            wgt_node._contextualize(self, wgt_name)
            wgt_node._instantiate()
            last_selection = wgt_name

        return _dataflow_at_selection(self, last_selection)

    def at(self, selection_name: str):
        if selection_name not in self.selections:
            raise KeyError(f"selection '{selection}' not found in dataflow.")
        return _dataflow_at_selection(self, selection_name)

    def get(self, query_defn):
        # issue new lazy<query> node everytime so existing definitions can be recycled later
        query_node = query(query_defn)
        query_node._contextualize(self)
        # return the (not yet instantiated) result node
        return result(query_node)

    def __setitem__(self, column_name, column_node):
        if isinstance(column_node, str):
            column_node = column.expression(column_node)
        column_node._contextualize(self, column_name)
        column_node._instantiate()

    # DSL syntax
    __lshift__ = input
    __matmul__ = at
    __and__ = filter
    __mul__ = weight
    __rshift__ = get

    analysis = _analysis
    output = _output

class _dataflow_at_selection:
    """
    Restricted view when operating at a specific selection.

    Only the following operations are allowed:
        - filter  (&)
        - weight  (*)
        - get     (>>)
    """

    _allowed_methods = {"filter", "weight", "get"}

    def __init__(self, df, selection):
        self._df = df
        self._selection = selection

    def _activate(self):
        self._df.current_selection = self._df.selections[self._selection]

    # Explicitly allow only specific methods
    def __getattr__(self, name):
        if name in self._allowed_methods:
            orig_method = getattr(self._df, name)

            def wrapper(*args, **kwargs):
                self._activate()
                return orig_method(*args, **kwargs)

            return wrapper

        raise AttributeError(
            f"Operation '{name}' is not allowed at a specific selection. "
            f"Only {self._allowed_methods} are permitted."
        )

    def __and__(self, cuts):
        return self.filter(cuts)

    def __mul__(self, weights):
        return self.weight(weights)

    def __rshift__(self, query_defn):
        return self.get(query_defn)