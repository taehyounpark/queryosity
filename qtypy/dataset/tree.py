from ..node import lazy

class tree(lazy):
    """
    qtypy layer for `qty::dataset::input<qty::ROOT::tree>`.

    Parameters
    ----------
    file_paths : list of str
        List of paths to the ROOT files containing the tree(s).
    tree_name : str
        Name of the tree inside the ROOT file(s) to access.
    """

    def __init__(self, file_paths, tree_name):
        super().__init__()

        self.file_paths = file_paths
        self.tree_name = tree_name

    def contextualize(self, df):
        df.dataset = self

        self.df = df
        self.instantiate()

    @property
    def cpp_initialization(self) -> str:
        file_paths_braced = '{' + ', '.join(['"{}"'.format(fp) for fp in self.file_paths]) + '}'
        tree_name_quoted = '"{}"'.format(self.tree_name)
        return "{df_id}.load(qty::dataset::input<qty::ROOT::tree>(std::vector<std::string>{file_paths}, std::string({tree_name})))".format(
                cpp_id=self.cpp_identifier,
                df_id=self.df.cpp_identifier,
                file_paths=file_paths_braced,
                tree_name=tree_name_quoted
            )