from ..node import column

class column(column):
    """
    Read a column from a loaded dataset.

    Parameters
    ----------
    key : str
        The name of the column in the dataset.
    cpp_value_type : type or str
        The C++ data type of the column values.
    """
    def __init__(self, key, dtype):
        super().__init__()
        self.key = key
        self.dtype = dtype

        self.dataset_name = None

    @property
    def cpp_value_type(self):
        return self.dtype

    def read(self, dataset_name):
        self.dataset_nname = dataset_name
        return self

    def __lshift__(self, dataset_name):
        return self.read(dataset_name)

    def __str__(self):
        return f'{self.cpp_value_type} ← "{self.key}"'

    @property
    def cpp_initialization(self):
        return '{dataset_id}.read(qty::dataset::column<{cpp_value_type}>("{key}"))'.format(
                dataset_id=self.df.dataset.cpp_identifier,
                cpp_value_type=self.cpp_value_type,
                key=self.key
            )