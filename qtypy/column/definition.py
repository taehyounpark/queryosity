import ROOT
import cppyy

from ..node import column

from functools import wraps
from typing import Callable, Any, List

class Definition(column):
    """
    Represents a C++-backed column node in the dataflow graph.

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
        self.defn: str = defn
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

    def contextualize(self, df: Any, name: str):
        """
        Inject the DataFrame/graph and instantiate the C++ slot.

        Applies the slot_hook function (if any) to all slot instances.
        """
        self.df = df

        self.instantiate()
        
        cppyy.cppdef('''
            auto {id}_slots = {id}.get_slots();
                                      '''.format(id=self.cpp_identifier))
        slots = getattr(cppyy.gbl, f'{self.cpp_identifier}_slots')

        if self.slot_hook is not None:
            for slot in slots:
                self.slot_hook(slot)

        df.columns[name] = self

def definition_decorator(cpp_class: str, column_args: List[str]):
    """
    Decorator for attaching a single slot_hook function to a C++-backed column.

    Example:

        @column.definition("OptimalCombinedTrigger", ["dijet_pTavg"])
        def set_triggers(slot, triggers, thresholds):
            slot.m_offlinePt99.push_back((thresholds[0], triggers[0]))

        df | {"optimalCombinedTrigger": set_triggers(triggers, thresholds)}
    """
    def decorator(user_func: Callable):
        @wraps(user_func)
        def factory(*user_args, **user_kwargs) -> "definition":
            col = Definition(cpp_class, column_args)

            if col.slot_hook is not None:
                raise RuntimeError("Column already has an slot_hook assigned")

            def slot_hook(slot: Any):
                user_func(slot, *user_args, **user_kwargs)

            col.slot_hook = slot_hook
            return col

        return factory

    return decorator
