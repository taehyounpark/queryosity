from .constant import constant
from .expression import expression
from .definition import Definition, definition_decorator
from .systematic import systematic
from .to_numpy import to_numpy

def definition(*args, **kwargs):
    return definition_decorator(*args, **kwargs)

definition.__doc__ = definition_decorator.__doc__
definition.__name__ = "definition"

__all__ = ["constant", "expression", "definition", "expression", "systematic", "to_numpy"]