# Installation

- Core requirements:
  - C++17
- Optional dependencies: 
  - [nlohmann::json](https://json.nlohmann.me)
  - [radpidcsv](https://github.com/d99kris/rapidcsv) 
  - [boost::histogram](https://www.boost.org/doc/libs/1_84_0/libs/histogram/doc/html/index.html)
- The following environments are included in CI:

| OS | Compiler | Versions |
| :--- | :--- | :--- |
| macOS 12 | Clang | 13.1, 13.2.1, 13.3.1, 13.4.1, 14.0, 14.0.1, 14.1 |
| Ubuntu 22.04 LTS | GCC | 9.4, 10.5, 11.4, 12.3 |

## Single-header

```cpp
#include "queryosity.h"
```

## CMake

```sh
git clone https://github.com/taehyounpark/queryosity.git
```

### Standalone

```sh
cd queryosity/ && mkdir build/ && cd build/
cmake -DQUERYOSITY_INSTALL=ON ../
cmake --build .
cmake --install .
```

```cmake
find_package(queryosity 0.4.1 REQUIRED)
...
add_library(MyAnalysis ...)
...
target_link_libraries(MyAnalysis INTERFACE queryosity::queryosity)
```

```cpp
#include "queryosity.h"
```

### Integrated

```cmake
add_subdirectory(queryosity)
...
add_library(MyAnalysis ...)
...
target_link_libraries(MyAnalysis INTERFACE queryosity::queryosity)
```

```cpp
#include "queryosity.h"
```
