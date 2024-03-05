# Queryosity

![Version](https://img.shields.io/badge/Version-0.2.0-blue.svg)
[![Ubuntu](https://github.com/taehyounpark/queryosity/actions/workflows/ubuntu.yml/badge.svg?branch=master)](https://github.com/taehyounpark/queryosity/actions/workflows/ubuntu.yml)
[![macOS](https://github.com/taehyounpark/queryosity/actions/workflows/macos.yml/badge.svg?branch=master)](https://github.com/taehyounpark/queryosity/actions/workflows/macos.yml)
[![Documentation](https://img.shields.io/badge/Documentation-mkdocs-blue.svg)](https://taehyounpark.github.io/queryosity/home/design/)
[![MIT License](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)

`queryosity` is powerful data analysis library written in C++.

<p align="center">
	<strong> <a href="https://taehyounpark.github.io/queryosity/">See full documentation</a></strong>
</p>


## Features

- *Dataflow* interface.
- Lazy actions.
- Multithreading.
- Sensitivity analysis.

## Design goals

- **Clear interface.** Use a clear high-level abstraction layer with modern C++ syntax to specify even the most complex actions.
- **Customizable actions.** Support for custom datasets and queries, as well as arbitrary computations in-between.
- **Sensitivity analysis.** Systematic variations within an analysis are automatically propagated and simultaneously processed.
- **Computational efficiency.** Actions are performed for an entry only when required. Dataset traversal is multithreaded.

## Installation

### [Single-header](https://raw.githubusercontent.com/taehyounpark/queryosity/master/queryosity.h)
```cpp
#include "queryosity.h"
```
### CMake
```sh
git clone https://github.com/taehyounpark/queryosity.git
``````
#### External
```sh
cd queryosity/ && mkdir build/ && cd build/
cmake ../
cmake --build .
cmake --install .
```
```cmake
find_package(queryosity 0.1.0 REQUIRED)
...
add_library(Analysis ...)
...
target_link_libraries(Analysis INTERFACE queryosity::queryosity)
```
```cpp
#include "queryosity/queryosity.h"
```
#### Integrated
```cmake
add_subdirectory(queryosity)
...
add_library(Analysis ...)
...
target_link_libraries(Analysis INTERFACE queryosity::queryosity)
```
