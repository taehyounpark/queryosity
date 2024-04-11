import os
import cppyy
import awkward as ak
import akarray

module_dir = os.path.dirname(__file__)
queryosity_h = os.path.join(module_dir, "queryosity.h")
ak_h = os.path.join(module_dir, "ak.h")
cppyy.include(queryosity_h)
cppyy.include(ak_h)
qty = cppyy.gbl.queryosity

# trigger queryosity to be loaded
df = qty.dataflow()
cppyy.cppdef(
    """
using dataflow = qty::dataflow;
namespace multithread = qty::multithread;
namespace dataset = qty::dataset;
namespace column = qty::column;
namespace selection = qty::selection;
namespace query = qty::query;
namespace systematic = qty::systematic;
"""
)


class LazyNode:
    def __init__(self, node):
        self._nd = node

    def filter(self, expression=None):
        pass

    def weight(self, expression=None):
        pass

    def fill(self, *columns):
        pass

    def at(self, *selections):
        pass


class LazyFlow:
    def __init__(self, multithread=False):
        self._df = qty.dataflow()
        self._columns = []
        self._selections = {}
        self._queries = []

    def load(self, *, dataset: dict = {}):
        pass

    def read(self, *, dataset: dict = {}, columns: dict = {}, awkward=None):
        if not awkward is None:
            # print(cppyy.gbl.std.make_unique[qty.ak.view[awkward.cpp_type]](awkward))
            ds = self._df._load[qty.ak.view[awkward.cpp_type]](cppyy.gbl.std.make_unique[qty.ak.view[awkward.cpp_type]](awkward))
            return ds.read(qty.dataset.column[cppyy.gbl.queryosity.ak.row_t[awkward.cpp_type]](""))

    def define(self, constant=None):
        if constant:
            cnst = self._df._assign[type(constant)](constant)
            self._columns.append(cnst)
            return cnst

    def filter(self, column=None, *, constant=None, expression=None, definition=None):
        if column:
            return LazyNode(self._df._cut(column))

    def weight(self, column=None, *, constant=None, expression=None, definition=None):
        pass

    def get(self, query: dict = {}):
        pass


if __name__ == "__main__":
    df = LazyFlow()
    c = df.define(constant=1.0)
    print(c)

    _df = df._df
    # print(c.initialize())
    s = df.filter(c)
    print(s)

    arr = ak.Array(
        [
            {"x": 1, "y": [1.1]}, {"x": 2, "y": [2.2, 0.2]},
            {},
            {"x": 3, "y": [3.0, 0.3, 3.3]},
        ]
    )

    arr = df.read(awkward=arr)
    print(arr)
