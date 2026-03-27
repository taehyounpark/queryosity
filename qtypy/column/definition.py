import cppyy

from ..node import column

from functools import wraps
from typing import Callable, Any, List

class Definition(column):
    """
    Represents a C++-backed column node in the dataflow.

    Attributes
    ----------
    defn : str
        Name of the C++ class to instantiate.
    args : list[str]
        List of column argument names.
    slot_hook : Callable[[Any], None] | None
        A single Python slot_hook to run on the C++ slot after instantiation.
    df : Any
        The parent DataFrame or graph object; injected via contextualize().
    """

    def __init__(self, defn: str, args: List[str]):
        super().__init__()
        if isinstance(defn, str):
            self.defn = defn
        else:
            self.defn = type(defn).__cpp_name__
        self.args: List[str] = args
        self.slot_hook: Callable[[Any], None] | None = None

    @property
    def cpp_value_type(self) -> str:
        return f"qty::column::value_t<typename decltype({self.cpp_identifier})::action_type>"

    @property
    def cpp_initialization(self) -> str:
        lazy_args = [self.df.columns[col].cpp_identifier for col in self.args]
        return (
            f"{self.df.cpp_identifier}.define(qty::column::definition<{self.defn}>())"
            f".evaluate({', '.join(lazy_args)})"
        )

    def __str__(self):
        return self.defn + "(" + ", ".join(self.args) + ")"

    def _contextualize(self, df: Any, name: str):
        """
        Inject the DataFrame/graph and instantiate the C++ slot.

        Applies the slot_hook function (if any) to all slot instances.
        """
        self.df = df

        self._instantiate()

        def apply_slot_hook():
            # need to separately get its slots for user-block
            cppyy.cppdef('''
                auto {id}_slots = {id}.get_slots();
                                        '''.format(id=self.cpp_identifier))
            slots = getattr(cppyy.gbl, f'{self.cpp_identifier}_slots')

            if self.slot_hook is not None:
                for slot in slots:
                    self.slot_hook(slot)

        df.columns[name] = self
        df.py_slot_hooks[len(df.cpp_lines)] = apply_slot_hook

def definition_decorator(cpp_class: str, cpp_ctor_args=()):
    """
    Declare a C++-backed custom column in the dataflow.

    Usage::
    
        args = []
        kwargs = {}

        @column.definition("CppClassName", (cpp_ctor_args...))("col1", "col2")
        def user_block(slot, *args, **kwargs):
            ...
        
        df["custom_column"] = user_block(*args, **kwargs)

    Overview:
        - The first argument to ``column.definition`` is the name of the
          concrete C++ class implementing the
          ``qty::column::definition`` interface.

          - This class must be compiled and have a generated dictionary so it is
            accessible via PyROOT bindings.
          - You can verify availability with::

                import ROOT
                hasattr(ROOT, "CppClassName")

        - The optional second argument is a tuple of constructor arguments
          for the C++ class. If omitted, the default constructor is used.

          - All constructor arguments are passed as strings to the Cling interpreter.
          - For non-trivial types, the string must be a valid C++ expression.

            Examples:
                - Pass a string / ``std::string``::

                      '"arg"'

                - Pass a vector from Python::

                      f'std::vector<double>{{{", ".join(str(v) for v in vec)}}}'

          - In practice, it is often simpler to rely on the default constructor
            and perform configuration in the Python ``user_block``.

        - The second function call specifies the names of input columns
          from the dataflow.

          - These columns must already exist.
          - They are passed as ``qty::column::observable`` objects to the
            C++ class ``evaluate()`` method.

        - The decorated function (``user_block``) is invoked on the
          instantiated C++ object(s).

          - The first argument (``slot``) is the C++ instance.
          - Additional arguments are user-defined.
          - This function is executed after instantiation and connection
            to the dataflow, but before the event loop starts.
          - This is the recommended place to configure the C++ instance
            (e.g., set member variables), making constructor arguments
            often unnecessary.

    Args:
        cpp_class (str): Name of the C++ class.

        cpp_ctor_args (tuple, optional): Constructor arguments passed to the
            C++ class as strings.

    Returns:
        Callable: A decorator that produces a ``Definition`` column.
    """

    # Normalize None → empty tuple
    if cpp_ctor_args is None:
        cpp_ctor_args = ()

    def column_args_decorator(*column_args):

        def decorator(user_func):

            @wraps(user_func)
            def factory(*user_args, **user_kwargs):

                # Pass ctor args into Definition
                col = Definition(
                    cpp_class,
                    list(column_args),
                    *cpp_ctor_args
                )

                if col.slot_hook is not None:
                    raise RuntimeError("Column already has a slot_hook assigned")

                def slot_hook(slot):
                    user_func(slot, *user_args, **user_kwargs)

                col.slot_hook = slot_hook
                return col

            return factory

        return decorator

    return column_args_decorator
