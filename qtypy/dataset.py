from . import lazynode 

class tree(lazynode):
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
        self.name = f'ds_{tree_name}'

        self.file_paths = file_paths
        self.tree_name = tree_name

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

class column(lazynode):
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

        self.dataset_name = None

    def read(self, dataset_name):
        self.dataset_nname = dataset_name
        return self

    def __lshift__(self, dataset_name):
        return self.read(dataset_name)

    def __str__(self):
        return f'{self.value_type} ← "{self.key}"'

    @property
    def cpp_initialization(self):
        ds = self.df.datasets[self.dataset_name] if self.dataset_name is not None else self.df.current_dataset
        return '{dataset_id}.read(qty::dataset::column<{value_type}>("{key}"))'.format(
                dataset_id=ds.cpp_identifier,
                value_type=self.value_type,
                key=self.key
            )