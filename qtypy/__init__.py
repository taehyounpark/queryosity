import ROOT
ROOT.gInterpreter.ProcessLine("""
#pragma GCC push_options
#pragma GCC optimize ("-O3")
#pragma GCC pop_options
""")
ROOT.queryosity.dataflow()

from .dataflow import dataflow
from . import column
from .query import hist, prof, hgrid

__all__: list[str] = [
    "analyze",
    "dataflow",
    "dataset",
    "column",
    "hist",
    "prof",
    "hgrid"
]