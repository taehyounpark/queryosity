from .cpputils import cpp_instantiable

class lazynode(cpp_instantiable):
    def __init__(self):
       super().__init__() 
       self.df = None
       self.cpp_prefix += 'lazy_'