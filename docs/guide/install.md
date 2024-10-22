# Installation

- Core requirement: C++17
- The following environments are included in CI:

| OS | Compiler | Versions |
| :--- | :--- | :--- |
| macOS 12 | Clang | 13.1, 13.2.1, 13.3.1, 13.4.1, 14.0, 14.0.1, 14.1 |
| Ubuntu 22.04 LTS | GCC | 9.4, 10.5, 11.4, 12.3 |

## Single-header

```cpp
#include <queryosity.hpp>
```

## CMake

```sh
git clone https://github.com/taehyounpark/queryosity.git
```

```sh
cd queryosity/ && mkdir build/ && cd build/
cmake -DQUERYOSITY_INSTALL=ON ../
cmake --build .
cmake --install .
```

| CMake flag | Default | Description |
| --- | --- | --- |
| `-DQUERYOSITY_INSTALL=ON` | `OFF` | Install the library so it can be found by a different CMake project. |
| `-DQUERYOSITY_BACKENDS=ON` | `OFF` | Compile pre-existing backends for input datasets & output queries. |
| `-DQUERYOSITY_TESTS=ON` | `OFF` | Compile tests. |
| `-DQUERYOSITY_EXAMPLES=ON` | `OFF` | Compile examples. |

```cmake
find_package(queryosity 0.5.0 REQUIRED)
...
add_library(MyAnalysis ...)
...
target_link_libraries(MyAnalysis INTERFACE queryosity::queryosity)
```

```cpp
#include <queryosity.hpp>
```

## Optional dependencies

Several backend implementations are used in examples throughout the documentation, which are also available for general use.
In order to enable them, the project should be configured with `-DQUERYOSITY_BACKENDS=ON` option set.

| Input | Output |
| :-- | --: |
| [radpidcsv](https://github.com/d99kris/rapidcsv)  | [boost::histogram](https://www.boost.org/doc/libs/1_86_0/libs/histogram/doc/html/index.html) |
| [nlohmann::json](https://json.nlohmann.me) | [ROOT::TH1](https://root.cern.ch/doc/master/classTH1.html) |
| [ROOT::TTree](https://root.cern.ch/doc/v630/classTTree.html) | [ROOT::TTree](https://root.cern.ch/doc/v630/classTTree.html) |