import ROOT
import cppyy
cppyy.cppdef("""
#if GCC_OPTIMIZE_AWARE
#pragma GCC push_options
#pragma GCC optimize ("-O3")
#pragma GCC pop_options
#endif
""")
from ROOT import queryosity as qty
qty.dataflow()

from .dataflow import dataflow