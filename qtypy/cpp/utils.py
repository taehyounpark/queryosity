import re
import hashlib

from tree_sitter import Language, Parser, Query, QueryCursor
import tree_sitter_cpp

CPP_LANGUAGE = Language(tree_sitter_cpp.language())
cpp_parser = Parser(CPP_LANGUAGE)

def find_cpp_identifiers(expr_str) -> list:

    expr_bytes = bytes(expr_str, 'utf8')
    tree = cpp_parser.parse(expr_bytes)
    root = tree.root_node
    identifiers = []

    def walk(node):
        if node.type == "identifier":
            identifiers.append(expr_bytes[node.start_byte:node.end_byte].decode())
        for child in node.children:
            walk(child)
    walk(root)
    return list(set([str(name) for name in identifiers]))

def generate_cpp_name(name: str) -> str:
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