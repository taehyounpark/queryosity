import os
import cppyy
import awkward as ak

module_dir = os.path.dirname(__file__)
queryosity_h = os.path.join(module_dir, "queryosity.h")
ak_h = os.path.join(module_dir, "ak.h")
cppyy.include(queryosity_h)
cppyy.include(ak_h)
qty = cppyy.gbl.queryosity

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
        if not len(dataset) == 1:
            raise ValueError(
                "dataset specification can only have one (key, value) pair: the dataset input format and constructor arguments."
            )
        ds_type = next(iter(dataset))
        ds_args = dataset[ds_type]
        return self._df._load[ds_type](cppyy.gbl.std.make_unique[ds_type](*ds_args))

    def read(self, *, dataset: dict = {}, columns: dict = {}):
        pass

    def define(self, *, constant=None):
        if constant:
            cnst = self._df._assign[type(constant)](constant)
            self._columns.append(cnst)
            return cnst

    def vary(self, *, nominal=None, constsant=None, expression=None, definition=None):
        pass

    def filter(self, column=None, *, constant=None, expression=None, definition=None):
        if column:
            return LazyNode(self._df._cut(column))

    def weight(self, column=None, *, constant=None, expression=None, definition=None):
        pass

    def get(self, query: dict = {}):
        pass


def from_awkward(arrays, **kwargs):
    df = LazyFlow(**kwargs)
    cols = []
    for array in arrays:
        ds = df.load(dataset={qty.ak.view[array.cpp_type]: [array]})
        cols.append(
            ds.read(
                qty.dataset.column[cppyy.gbl.queryosity.ak.record_t[array.cpp_type]]("")
            )
        )
    return df, cols

if __name__ == "__main__":
    
    arr = ak.Array(
        [
            [{"x": 1, "y": [1.1]},
            {"x": 2, "y": [2.2, 0.2]}],
            [{}],
            [{"x": 3, "y": [3.0, 0.3, 3.3]}],
        ]
    )

    print(arr.cpp_type)
    df, [arr] = from_awkward([arr], multithread=True)
    print(arr)