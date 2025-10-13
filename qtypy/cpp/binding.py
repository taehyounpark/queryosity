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

class cpp_binding:

    def __init__(self):
        self.name = None
        self.cpp_prefix = '__qtypy__'

    @cached_property
    def cpp_identifier(self):
        return f'{self.cpp_prefix}{cpp_name(self.name)}'

    @property
    def cpp_instance(self):
        return getattr(cppyy.gbl, self.cpp_identifier, None)

# class cpp_instance:
#     pass
