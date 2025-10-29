import re
import hashlib

from tree_sitter import Node, Language, Parser
import tree_sitter_cpp

CPP_LANGUAGE = Language(tree_sitter_cpp.language())
cpp_parser = Parser(CPP_LANGUAGE)

def find_cpp_identifiers_and_replace_with_values(expr: str, candidates: set):
    """
    Parse a C++ expression, extract identifiers that match candidates,
    and return:
      - column_args: list of matching identifiers
      - new_expr: expression with column identifiers replaced with .value()
    """

    expr_bytes = expr.encode("utf8")
    tree = cpp_parser.parse(expr_bytes)
    root = tree.root_node

    column_args = set()
    replacements = []

    def walk(node):
        if node.type == "identifier":
            name = expr_bytes[node.start_byte:node.end_byte].decode()
            if name in candidates:
                column_args.add(name)
                replacements.append((node.start_byte, node.end_byte, f"{name}.value()"))
        for child in node.children:
            walk(child)

    walk(root)

    # Apply replacements from end to start
    new_expr = expr
    for start, end, repl in sorted(replacements, reverse=True):
        new_expr = new_expr[:start] + repl + new_expr[end:]

    return list(column_args), new_expr

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