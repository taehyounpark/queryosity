The following compilers with C++17 support are part of the CI.

| OS | Compiler | Versions |
|:---|:--|:--|
| macOS 12 | Clang | 13.1, 13.2.1, 13.3.1, 13.4.1, 14.0, 14.0.1, 14.1 |
| ubuntu 22.04 LTS | GCC | 8.4.0, 9.4.0, 10.5.0, 11.4.0, 12.3.0 |

## Single-header
The single-header file can be downloaded from [here](https://raw.githubusercontent.com/taehyounpark/analogical/master/analogical.h).
```cpp
#include "analogical.h"
```
## CMake
```sh
git clone https://github.com/taehyounpark/analogical.git
```
### External
```sh
cd analogical/ && mkdir build/ && cd build/
cmake ../
cmake --build .
cmake --install .
```
```CMake
find_package(analogical 0.1.0 REQUIRED)
...
add_library(Analysis ...)
...
target_link_libraries(Analysis INTERFACE ana::analogical)
```
```cpp
#include "ana/analogical.h"
```
### Integrated
```CMake
add_subdirectory(analogical)
...
add_library(Analysis ...)
...
target_link_libraries(Analysis INTERFACE ana::analogical)
```
