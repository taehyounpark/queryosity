
from ..node import column

class systematic(column):
    def __init__(self, nominal : column, variations : dict[str, column]):
        super().__init__()
        self.nominal = nominal
        self.variations = variations

    def contextualize(self, df, name):
        if name in df.columns:
            raise ValueError("column already exists")
        df.columns[name] = self

        # set dataflow link for everything
        self.df = df
        self.nominal.df = df
        for variation in self.variations.values():
            variation.df = df

    def instantiate(self):
        # instantiate lazy<column> nodes first
        self.nominal.instantiate()
        for variation in self.variations.values():
            variation.instantiate()
        # instantiate varied<lazy> node
        super().instantiate()

    @property
    def cpp_initialization(self):
        df_id = self.df.cpp_identifier 

        # format nominal part
        nominal_part = f"qty::column::nominal({self.nominal.cpp_identifier})"

        # format variations as {{"name1", id1}, {"name2", id2}, ...}
        variations_part = ", ".join(f'{{"{name}", {var.cpp_identifier}}}' for name, var in self.variations.items())
        variations_part = f"{{{variations_part}}}"  # wrap in outer braces

        # combine into df.vary call
        return f"{df_id}.vary({nominal_part}, {variations_part});"

    @property
    def cpp_value_type(self) -> str:
        return f'qty::column::value_t<typename decltype({self.nominal.cpp_identifier})::action_type>'
