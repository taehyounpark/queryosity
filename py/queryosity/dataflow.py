import os
import cppyy

# from .lazy import LazyNode
# from .todo import TodoItem

queryosity_h = os.path.join(os.path.dirname(os.path.abspath(__file__)),'queryosity.h')
cppyy.include(queryosity_h)

qty = cppyy.gbl.queryosity
dataflow = qty.dataflow
dataset = qty.dataset
multithread = qty.multithread
column = qty.column
query = qty.query
systematic = qty.systematic

class LazyFlow(object):
  
  def __init__(self, *, mt=1, nrows=-1, weight=1.0):
    if mt is True: mt = -1
    self._df = dataflow(cppyy.gbl.std.move(multithread.enable(mt)), cppyy.gbl.std.move(dataset.weight(weight)))

  def load(self, dataset):
    return self._df.load(dataset)

  def define(self, *, definition=None, expression=None, constant=None, observables=[]):
    return None

  def filter(self, column=None, *, expression=None):
    return None

  def weight(self, column=None, *, expression=None):
    return None

  def make(self):
    return None