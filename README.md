***Ana**lysis **Logic** **A**bstraction **L**ayer*

![Version](https://img.shields.io/badge/Version-0.1.1-blue.svg)
[![Ubuntu](https://github.com/taehyounpark/analogical/actions/workflows/ubuntu.yml/badge.svg?branch=master)](https://github.com/taehyounpark/analogical/actions/workflows/ubuntu.yml)
[![macOS](https://github.com/taehyounpark/analogical/actions/workflows/macos.yml/badge.svg?branch=master)](https://github.com/taehyounpark/analogical/actions/workflows/macos.yml)
[![MIT License](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)


## Introduction

`analogical` is a C++ library for dataset transformation.

Its key features include:

- "Dataflow" interface.
- Lazy execution.
- Multithreaded processing.
- Sensitivity analysis.

## Design goals

- **Clear interface.** Higher-level languages have a myriad of libraries available to do intuitive and efficient data analysis. The syntax here aims to achieve a similar level of abstraction in its own way.
- **Interface-only.** No implementation of a data formats or aggregation output is provided out-of-the-box. Instead, the interface allows defining operations with arbitrary inputs, execution, and outputs as needed.
- **Sensitivity analysis.** Often times, changes to an analysis need to be explored for sensitivity analysis. How many times has this required the dataset to be re-processed? With built-in handling of systematic variations, changes can their impacts retrieved all together.
- **Computational efficiency.** All operations within the dataset processing is performed at most once per-entry, only when needed. All systematic variations are processed at once. The dataset processing is multithreaded for thread-safe plugins.

## Documentation

***<p style="text-align: center;">[The documentation is under construction](https://taehyounpark.github.io/analogical/)</p>***


## Installation

Requirements: Unix OS, C++17

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
