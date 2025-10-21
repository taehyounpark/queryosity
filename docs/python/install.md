# Installation

The recommended way is to compile the library from source and enable the option

```sh
cmake -DQUERYOSITY_PYTHON=ON </path/to/queryosity>
```

which will ensure that both the C++ backend and Python layer are compatible with each other, and run `pip` automatically. Alternatively, you can install the package (along with the CLI executable) by

```sh
pip install -e .
export PATH=${PATH}:</follow/pip/instructions>
```
