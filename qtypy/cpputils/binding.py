import ROOT, cppyy

from abc import ABC, abstractmethod
from functools import cached_property

import re
import hashlib

def cpp_name(name: str) -> str:
    """
    Convert a string to a valid C++ identifier, with a short hash suffix.
    Example:
        'my object@!#' → 'my_object_3f2a1b'
    """
    # Replace invalid characters with underscores
    name_safe = re.sub(r'[^a-zA-Z0-9_]', '_', name)
    # Add a short deterministic hash to avoid collisions
    hash_digest = hashlib.md5(name_safe.encode()).hexdigest()[:6]
    return f"{name_safe}_{hash_digest}"

_global_instance_count = 0

class cpp_instantiable(ABC):
    def __init__(self):
        self._instantiated = False
        # Use the global counter
        global _global_instance_count
        _global_instance_count += 1
        self._instance_num = _global_instance_count  # Store instance number

        self.cpp_typename = 'auto'
        self.cpp_prefix = '__qtypy__'
        self.name = None

    @cached_property
    def cpp_identifier(self):
        identifier = f'{self.cpp_prefix}{self.name}_{self._instance_num}'
        return identifier

    @property
    @abstractmethod
    def cpp_initialization(self) -> str:
        pass

    @property
    def cpp_instance(self):
        self.instantiate()
        return getattr(cppyy.gbl, self.cpp_identifier, None)

    def instantiate(self):
        if not self._instantiated:
            cppyy.cppdef('''{type} {id} = {init};'''.format(
                type = self.cpp_typename,
                id = self.cpp_identifier,
                init = self.cpp_initialization
            ))
        self._instantiated = True

# class cpp_instance:
#     pass