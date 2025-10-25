import ROOT
import cppyy
cppyy.cppdef("""
#pragma GCC push_options
#pragma GCC optimize ("-O3")
#pragma GCC pop_options
""")
ROOT.queryosity.dataflow()

# ...existing code...
import importlib
from typing import Any, List

# Names you want to expose from the package (adjust as needed)
__all__: List[str] = [
    "analyze",
    "dataflow",
    "dataset",
    "lazy",
    "query",
    "column",
    "cpputils"
    # add other package-level exports here
]

def _load_submodule(name: str) -> Any:
    # import the submodule (e.g. qtypy.dataflow) and if the submodule
    # defines an attribute with the same name return that attribute,
    # otherwise return the module object
    mod = importlib.import_module(f"{__name__}.{name}")
    return getattr(mod, name) if hasattr(mod, name) else mod

def __getattr__(name: str) -> Any:
    if name in __all__:
        val = _load_submodule(name)
        globals()[name] = val  # cache for subsequent access
        return val
    raise AttributeError(f"module {__name__!r} has no attribute {name!r}")

def __dir__() -> List[str]:
    return sorted(list(globals().keys()) + __all__)