from queryosity import LazyFlow
from queryosity import jit

@jit(['int'], 'int')
def testing(x): return x+1

print(testing)