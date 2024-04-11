import awkward as ak
import awkward._connect.cling

import cppyy
cppyy.include("ak.h")

arr = ak.Array(
    [
        [{"x": 1, "y": [1.1]}, {"x": 2, "y": [2.2, 0.2]}],
        [],
        [{"x": 3, "y": [3.0, 0.3, 3.3]}],
    ]
)
# arr

# print(arr.cpp_type)
# print(arr.numba_type)
# print(arr.__dir__())
# print(arr[0].cpp_type)

# view = cppyy.gbl.queryosity.akarray.view[arr.cpp_type](arr)
# print(view)

# row = view.read[float](1,"")
# print(row)
# print(cppyy.gbl.queryosity.akarray.row_t[arr.cpp_type])
# print(row.__dir__())
# v = row.read(0,0)
# print(v.__dir__())
# print(v[0].x())
# print(row.field())