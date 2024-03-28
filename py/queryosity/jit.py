import os
import cppyy
import cppyy.numba_ext
import numba
from numba import njit

cppfunc_h = os.path.join(os.path.dirname(os.path.abspath(__file__)),'cppfunc.h')
cppyy.include(cppfunc_h)
cppfunc = cppyy.gbl.queryosity.py.cppfunc

# Define the custom decorator with its own arguments
def jit(args=[], ret=''):

    if not len(args):
      raise ValueError("no argument types specified.")

    if not ret: 
      raise ValueError("return type not specified.")

    def decorator(func):
        # Apply njit with parallel=True to the function
        jitted_func = njit(parallel=True)(func)

        print(jitted_func.__dict__)
        
        # Example pre-processing logic using decorator arguments
        print("Before calling the njitted function")

        fn = cppfunc[ret,*args](func.__module__, func.__name__)

        # Example post-processing logic
        print("After calling the njitted function")
        # Modify the result or perform additional actions here
        
        return fn
    return decorator