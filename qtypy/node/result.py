from ..cpp import cpp_binding

class result(cpp_binding):
    def __init__(self, query):
        super().__init__()
        self.query = query
        self.name = f'result_{query.name}'

    @property
    def cpp_type(self):
        return f'{self.query.defn.cpp_result_type} const &'

    @property
    def cpp_initialization(self):
        return f'{self.query.cpp_identifier}.{self.query.defn.cpp_result_call}'

    def nominal(self):
        return nominal_result(self.query)

    def variation(self, variation_name : str):
        return varied_result(self.query, variation_name)

    def variations(self, variation_names : list[str]):
        return {variation_name : varied_result(self.query, variation_name) for variation_name in variation_names}

    def result(self):
        return self.query.defn.py_result_wrapper(self.cpp_instance)

class nominal_result(result):

    def __init__(self, query):
        super().__init__(query)
        self.name += f'_nominal'

    @property
    def cpp_initialization(self):
        return f'{self.query.cpp_identifier}.nominal().{self.query.defn.cpp_result_call}'
    
class varied_result(result):

    def __init__(self, query, variation : str):
        super().__init__(query)
        self.name += f'_nominal'
        self.variation_name = variation

    @property
    def cpp_initialization(self):
        return f'{self.query.cpp_identifier}.variation("{self.variation_name}").{self.query.defn.cpp_result_call}'

