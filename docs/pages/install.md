@page install Installation
@tableofcontents

- Requirement: C++17

| OS | Compiler | Versions |
| :--- | :--- | :--- |
| macOS 12 | Clang | 13.1, 13.2.1, 13.3.1, 13.4.1, 14.0, 14.0.1, 14.1 |
| Ubuntu 22.04 LTS | GCC | 9.4, 10.5, 11.4, 12.3 |

@section install-header Single-header

~~~{.cpp}
#include "queryosity.h"
~~~

@section install-cmake CMake

~~~{.sh}
git clone https://github.com/taehyounpark/queryosity.git
~~~

@subsection install-cmake-standalone Standalone

~~~{.sh}
cd queryosity/ && mkdir build/ && cd build/
cmake ../
cmake --build .
cmake --install .
~~~

~~~{.cmake}
find_package(queryosity 0.1.0 REQUIRED)
...
add_library(YourProject ...)
...
target_link_libraries(YourProject INTERFACE queryosity::queryosity)
~~~

~~~{.cpp}
#include "queryosity/queryosity.h"
~~~

@subsection install-cmake-integrated Integrated

~~~{.cmake}
add_subdirectory(queryosity)
...
add_library(YourProject ...)
...
target_link_libraries(YourProject INTERFACE queryosity::queryosity)
~~~
~~~{.cpp}
#include "queryosity/queryosity.h"
~~~
