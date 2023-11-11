_**Ana**lysis **Logic** **A**bstraction **L**ayer_

![Version](https://img.shields.io/badge/Version-0.2.0-blue.svg)
[![Ubuntu](https://github.com/taehyounpark/analogical/actions/workflows/ubuntu.yml/badge.svg?branch=master)](https://github.com/taehyounpark/analogical/actions/workflows/ubuntu.yml)
[![macOS](https://github.com/taehyounpark/analogical/actions/workflows/macos.yml/badge.svg?branch=master)](https://github.com/taehyounpark/analogical/actions/workflows/macos.yml)
[![Documentation](https://img.shields.io/badge/Documentation-mkdocs-blue.svg)](https://taehyounpark.github.io/analogical/home/design/)
[![MIT License](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)

`analogical` is a C++ library for performing dataset transformations.

<p align="center">
	<strong> <a href="https://taehyounpark.github.io/analogical/">See full documentation</a></strong>
</p>


## Features

- *Dataflow* interface.
- Lazy execution.
- Multithreaded processing.
- Sensitivity analysis.

## Design goals

- **Clear interface.** Use a clear high-level abstraction layer with modern C++ syntax to specify even the most complex operations.
- **Customizable operations.** Support for inputs and outputs of any type and arbitrary execution, such as custom dataset formats, column definitions, and aggregation algorithms.
- **Sensitivity analysis.** Systematic variations of an analysis are automatically propagated and simultaneously processed within one dataset traversal.
- **Computational efficiency.** Dataset operations are performed for an entry only if needed. Dataset traversal is multithreaded.

## Installation

### [Single-header](https://raw.githubusercontent.com/taehyounpark/analogical/master/analogical.h)
```cpp
#include "analogical.h"
```
### CMake
```sh
git clone https://github.com/taehyounpark/analogical.git
``````
#### External
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
#### Integrated
```cmake
add_subdirectory(analogical)
...
add_library(Analysis ...)
...
target_link_libraries(Analysis INTERFACE ana::analogical)
```
