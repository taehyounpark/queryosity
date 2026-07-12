# Installation

Python bindings are provided through `cppyy` shipped within `ROOT` library. To enable these, the package must be compiled from source with the following option enabled:

```sh
cmake -DQUERYOSITY_ROOT=ON </path/to/queryosity>
```

which will ensure that both the C++ backend and Python layer are compatible with each other, and run `pip` automatically. Alternatively, you can install the package (along with the CLI executable) by

```sh
pip install -e .
export PATH=${PATH}:</follow/pip/instructions>
```
