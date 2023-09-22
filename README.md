***Ana**lysis **Logic** **A**bstraction **L**ayer*

![Version](https://img.shields.io/badge/Version-0.1.1-blue.svg)
[![Ubuntu](https://github.com/taehyounpark/analogical/actions/workflows/ubuntu.yml/badge.svg?branch=master)](https://github.com/taehyounpark/analogical/actions/workflows/ubuntu.yml)
[![macOS](https://github.com/taehyounpark/analogical/actions/workflows/macos.yml/badge.svg?branch=master)](https://github.com/taehyounpark/analogical/actions/workflows/macos.yml)
[![MIT License](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)


## Introduction

`analogical` is a C++ interface for dataset transformations.

Its key features include:

- "DataFlow" paradigm of computational tasks within a directed graph.
- Lazy execution.
- Implicit multithreading.


## Design goals

- **Clear syntax.** Higher-level languages have myriad of libraries available to do columnar data analysis intuitively, e.g. "DataFrame". The syntax used here aims to achieve a similar level of abstraction in its own way, referred to as "DataFlow" here.
- **Interface-only.** No implementation of a data formats or aggregation output is provided out-of-the-box. Instead, the interface allows defining operations with arbitrary inputs, execution, and outputs as needed.
- **Non-proliferative workflow.** Often times, small changes to an analysis need to be explored. How many times has CTRL+C/V been used to copy an entire analysis, made minute changes, and re-process the dataset? With built-in handling of "systematic variations", such changes can be performed and retrieved simultaneously.
- **Computational efficiency.** All operations within the dataset processing is performed at most once per-entry, only when needed. All systematic variations are processed at once. The dataset processing is multithreaded for thread-safe plugins.


## Installation

C++17 support is required (tested with Clang 14.0.0 and GCC 9.3.0).

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
#### Integrated
```CMake
add_subdirectory(analogical)
...
add_library(Analysis ...)
...
target_link_libraries(Analysis INTERFACE ana::analogical)
```
