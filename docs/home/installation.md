The following compilers with C++17 support are part of the CI.

| OS | Compiler | Versions |
|:---|:--|:--|
| macOS 12 | Clang | 13.1, 13.2.1, 13.3.1, 13.4.1, 14.0, 14.0.1, 14.1 |
| Ubuntu 22.04 LTS | GCC | 9.4, 10.5, 11.4, 12.3 |

## [Single-header](https://raw.githubusercontent.com/taehyounpark/analogical/master/analogical.h)
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
```cmake
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
```cmake
add_subdirectory(analogical)
...
add_library(Analysis ...)
...
target_link_libraries(Analysis INTERFACE ana::analogical)
```
```cpp
#include "ana/analogical.h"
```
