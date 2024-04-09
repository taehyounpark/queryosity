# How to contribute

The sole developer of this library is the author, [Tae Hyoun Park](https://github.com/taehyounpark); this hopefully changes with more users and contributors.
Any bug reports, suggestions, or ideas are greatly appreciated and should be brought up by creating an issue.

## Making changes

1. Fork the repository.
2. Make your changes inside a topical branch `topic`.
3. Submit a pull request from `origin/topic` to `upstream/master`.

## Running tests

Unit tests for core functionalities of the library are enabled by 
```sh
cmake -DQUERYOSITY_TESTS=ON ../
```
They can be ran from the `build/tests/` directory by
```sh
ctest
```

## Possible areas of contributions

Contributions from users should prioritize whatever fixes/features they need.
Nevertheless, below are a few possibilities:

| Task | Description | Skills | Prerequisites |
|:--: |:--| :--: | :--: |
| 1 | Improve multithreading performance with [Intel TBB](https://www.intel.com/content/www/us/en/developer/tools/oneapi/onetbb.html#gs.7ombth). | C++ | |
| 2 | Develop a Python(ic) layer through [cppyy](https://cppyy.readthedocs.io/en/latest/). | C++ & Python | |
| 3 | Interoperate with [AwkwardArray](https://awkward-array.org/doc/main/user-guide/how-to-use-in-cpp.html) as input/output column. | C++ & Python | 2 |

Should anyone feel up to the challenge, please feel free to open an issue!
