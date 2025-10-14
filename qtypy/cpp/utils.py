from tree_sitter import Language, Parser, Query, QueryCursor
import tree_sitter_cpp

CPP_LANGUAGE = Language(tree_sitter_cpp.language())
cpp_parser = Parser(CPP_LANGUAGE)

def parse_cpp_identifiers(expr_str) -> list:
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
