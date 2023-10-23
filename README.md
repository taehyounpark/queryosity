***Ana**lysis **Logic** **A**bstraction **L**ayer*

![Version](https://img.shields.io/badge/Version-0.1.1-blue.svg)
[![Ubuntu](https://github.com/taehyounpark/analogical/actions/workflows/ubuntu.yml/badge.svg?branch=master)](https://github.com/taehyounpark/analogical/actions/workflows/ubuntu.yml)
[![macOS](https://github.com/taehyounpark/analogical/actions/workflows/macos.yml/badge.svg?branch=master)](https://github.com/taehyounpark/analogical/actions/workflows/macos.yml)
[![Documentation](https://img.shields.io/badge/mkdocs-Documentation-blue.svg)](https://opensource.org/licenses/MIT)
[![MIT License](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)


## Introduction

`analogical` is a C++ library for dataset transformation.

Its key features include:

- "Dataflow" interface.
- Lazy execution.
- Multithreaded processing.
- Sensitivity analysis.

## Design goals

- **Clear interface.** Higher-level languages have an abundance of available libraries to do intuitive and efficient data analysis. An interface with a similar level of abstraction with modern C++ syntax.
- **Customizable plugins.** Arbitrary operations with custom input(s), execution, and output(s) receive first-class treatment. From non-trivial datasets to complex computations and aggregations, there is an ABC available for implementation.
- **Sensitivity analysis.** With built-in handling of systematic variations, changes in operations can be processed *once* to retrieve all results under nominal and varied scenarios simultaneously.
- **Computational efficiency.** Operations within the dataset processing are performed at most once per-entry and only when needed. If enabled, the processing is multithreaded.

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
