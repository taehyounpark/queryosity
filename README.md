***Ana**lysis **Logic** **A**bstraction **L**ayer*

![Version](https://img.shields.io/badge/Version-0.2.0-blue.svg)
[![Ubuntu](https://github.com/taehyounpark/analogical/actions/workflows/ubuntu.yml/badge.svg?branch=master)](https://github.com/taehyounpark/analogical/actions/workflows/ubuntu.yml)
[![macOS](https://github.com/taehyounpark/analogical/actions/workflows/macos.yml/badge.svg?branch=master)](https://github.com/taehyounpark/analogical/actions/workflows/macos.yml)
[![Documentation](https://img.shields.io/badge/mkdocs-Documentation-blue.svg)](https://opensource.org/licenses/MIT)
[![MIT License](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)


## Features

`analogical` is a C++ dataset transformation library.

- "Dataflow" interface.
- Lazy execution.
- Multithreaded processing.
- Sensitivity analysis.

## Design goals

- **Clear interface.** Specify operations with a clear, high-level abstraction interface using modern C++ syntax.
- **Customizable plugins.** Operations with arbitrary inputs, execution, and outputs receive first-class treatment: from custom datasets and columns to complex aggregations, there is a customizable ABC.
- **Sensitivity analysis.** With built-in handling of systematic variations, changes to an operation are automatically propagated and all results under the original and varied scenarios are obtained simultaneously.
- **Computational efficiency.** Dataset operations are (1) multithreaded, and (2) performed for an entry only if needed.

## Documentation

***<p style="text-align: center;">[The documentation is under construction](https://taehyounpark.github.io/analogical/)</p>***


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
