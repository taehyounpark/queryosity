from .cpp import cpp_binding

class lazy(cpp_binding):
    def __init__(self):
       super().__init__() 
       self.df = None
       self.cpp_prefix += 'lazy_'