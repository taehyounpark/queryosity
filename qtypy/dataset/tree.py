from ..node import lazy

import ROOT

def get_branch_types(tfile):
    """
    Returns a dictionary where keys are branch names and values are their C++ types in the TTree.
    
    :param tfile: Opened TFile containing the TTree
    :return: Dictionary with branch names as keys and their C++ types as values
    """
    # Ensure that we have a valid file and TTree
    if not tfile or not tfile.IsOpen():
        raise ValueError("File not open or invalid TFile.")
    
    # Assuming the TTree is the first object in the file
    tree = tfile.GetListOfKeys()[0].ReadObj()  # or replace with your TTree's name if needed
    if not isinstance(tree, ROOT.TTree):
        raise TypeError("Object in file is not a TTree.")
    
    branch_types = {}
    
    # Loop over each branch in the TTree
    for branch in tree.GetListOfBranches():
        branch_name = branch.GetName()
        # Get the branch's class name (C++ type)
        branch_type = branch.GetClassName()
        branch_types[branch_name] = branch_type
    
    return branch_types

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

    def __init__(self, file_paths, tree_name, columns : list[str] = []):
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