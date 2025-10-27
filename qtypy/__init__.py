import ROOT
import cppyy
cppyy.cppdef("""
#pragma GCC push_options
#pragma GCC optimize ("-O3")
#pragma GCC pop_options
""")
ROOT.queryosity.dataflow()

from .dataflow import dataflow
from .selection import filter, weight
from .query import hist

__all__: list[str] = [
    "analyze",
    "dataflow",
    "dataset",
    "column",
    "filter",
    "weight"
    "hist"
]