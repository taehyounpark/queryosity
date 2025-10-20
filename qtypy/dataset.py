import cppyy

from .cpp import cpp_binding 

class tree(cpp_binding):
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
        self.name = 'ds'

        self.file_paths = file_paths
        self.tree_name = tree_name

    def instantiate(self, df):
        file_paths_braced = '{' + ', '.join(['"{}"'.format(fp) for fp in self.file_paths]) + '}'
        tree_name_quoted = '"{}"'.format(self.tree_name)
        return cppyy.cppdef(
            "auto {cpp_id} = {df_id}.load(qty::dataset::input<qty::ROOT::tree>(std::vector<std::string>{file_paths}, std::string({tree_name})));".format(
                cpp_id=self.cpp_identifier,
                df_id=df.cpp_identifier,
                file_paths=file_paths_braced,
                tree_name=tree_name_quoted
            )
        )

class column(cpp_binding):
    """
    Read a column from a loaded dataset.

    Parameters
    ----------
    key : str
        The name of the column in the dataset.
    value_type : type or str
        The C++ data type of the column values.
    """
    def __init__(self, key, value_type):
        super().__init__()
        self.key = key
        self.value_type = value_type

        self.cpp_prefix = '_df_column'
        self.name = None

    def __str__(self):
        return f'{self.value_type} <- "{self.key}"'

    def instantiate(self, df):
        return cppyy.cppdef(
            'auto {cpp_id} = {dataset_id}.read(qty::dataset::column<{value_type}>("{key}"));'.format(
                cpp_id=self.cpp_identifier,
                dataset_id=df.dataset.cpp_identifier,
                value_type=self.value_type,
                key=self.key
            )
        )