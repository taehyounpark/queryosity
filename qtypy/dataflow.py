from .cpp import cpp_binding
from . import dataset
from . import column
from . import selection
from .node import query, result

from rich.console import Console
from rich.panel import Panel
from rich.table import Table
from rich.tree import Tree
from rich.rule import Rule
from rich.text import Text

# ==========================================
# Configured Analysis
# ==========================================

class _custom_analysis:

    def __init__(self, func, args, kwargs):
        self.func = func
        self.args = args
        self.kwargs = kwargs

    def __ror__(self, df):
        # ignore returned, always return df
        self.func(df, *self.args, **self.kwargs)
        return df

    def output(self, df):
        # transparently return back out whatever the results are
        return self.func(df, *self.args, **self.kwargs)

# ==========================================
# Definition Objects (Decorators)
# ==========================================

class _analysis:
    def __init__(self, func):
        self.func = func

    def __call__(self, *args, **kwargs):
        return _custom_analysis(self.func, args, kwargs)

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
        self.current_selection_name = None
        self.queries = []

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
            raise KeyError(f"selection '{selection_name}' not found in dataflow.")
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

    def output(self, custom_analysis):
        return custom_analysis.output(self)

    def explain(self):

        console = Console()

        # ------------------------------------------------
        # DATASET BLOCK
        # ------------------------------------------------

        dataset_panel = Table.grid(padding=(0,1))

        dataset_panel.add_row(f"[grey]{self.dataset}[/grey]")

        # horizontal divider
        dataset_panel.add_row(Rule(style="dim"))

        # show dataset columns if they exist
        dataset_columns = {
            name : col 
            for name, col in self.columns.items()
            if isinstance(col, dataset.column)
        }

        if dataset_columns:
            for name, col in dataset_columns.items():
                dataset_panel.add_row(f"[grey]{name}{col}[/grey]")

        console.print(
            Panel(
                dataset_panel,
                title="[bold]dataset[/bold]",
                title_align="left",
                expand=False
            )
        )

        # ------------------------------------------------
        # COLUMN COMPUTATION BLOCK
        # ------------------------------------------------

        computation_panel = Table.grid(padding=(0,1))

        for name, col in self.columns.items():

            # skip dataset-native columns
            if isinstance(col, dataset.column):
                continue

            if name in self.selections:
                continue

            computation_panel.add_row(Text(f"{name} := {col}", style="blue"))

        console.print(
            Panel(
                computation_panel,
                title="[bold]observable[/bold]",
                title_align="left",
                expand=False
            )
        )

        # ------------------------------------------------
        # SELECTION TREE BLOCK
        # ------------------------------------------------

        # root node with a neutral symbol '*'
        cutflow_tree = Tree("[blue]*[/blue]")

        if not self.selections:
            cutflow_tree.add("[dim]none[/dim]")
            console.print(
                Panel(
                    cutflow_tree,
                    title="[bold]cutflow[/bold]",
                    title_align="left",
                    expand=False
                )
            )
            return

        # --------------------------------------------
        # Build parent → children map
        # --------------------------------------------

        children = {}
        roots = []

        for name, sel in self.selections.items():
            prev_name = getattr(sel, "prev_name", None)
            if prev_name is None:
                roots.append(name)
            else:
                children.setdefault(prev_name, []).append(name)

        # --------------------------------------------
        # Recursive builder
        # --------------------------------------------

        def build(node, name):
            sel = self.selections[name]
            branch = node.add(f"@ {name} → {sel}")
            for child in children.get(name, []):
                build(branch, child)

        for root in roots:
            build(cutflow_tree, root)

        # wrap the tree in a panel
        console.print(
            Panel(
                cutflow_tree,
                title="[bold]cutflow[/bold]",
                title_align="left",
                expand=False
            )
        )

        # ------------------------------------------------
        # QUERY TREE BLOCK
        # ------------------------------------------------

        # container grid (like your dataset block)
        query_panel = Table.grid(padding=(0, 1))
        # group queries by booked_selection_name
        queries_by_selection = {}

        for q in self.queries:
            sel_name = getattr(q, "booked_selection_name", None)
            queries_by_selection.setdefault(sel_name, []).append(q)

        # create flat roots (no hierarchy)
        for sel_name in self.selections.keys():

            sel_tree = Tree(f"[blue]@ {sel_name}[/blue]")

            queries = queries_by_selection.get(sel_name, [])
            if not queries:
                continue
            for q in queries:
                sel_tree.add(Text(f"{q.defn}", style="red"))  # relies on __str__()

            query_panel.add_row(sel_tree)

        console.print(
            Panel(
                query_panel,
                title="[bold]query[/bold]",
                title_align="left",
                expand=False
            )
        )

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
        self._selection_name = selection

    def _activate(self):
        self._df.current_selection_name = self._selection_name
        self._df.current_selection = self._df.selections[self._selection_name]

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