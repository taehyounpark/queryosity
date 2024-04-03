# Queryosity

![Version](https://img.shields.io/badge/Version-0.4.0-blue.svg)
![C++ Standard](https://img.shields.io/badge/C++-17-blue.svg)
[![Ubuntu](https://github.com/taehyounpark/analogical/actions/workflows/ubuntu.yml/badge.svg?branch=master)](https://github.com/taehyounpark/analogical/actions/workflows/ubuntu.yml)
[![macOS](https://github.com/taehyounpark/analogical/actions/workflows/macos.yml/badge.svg?branch=master)](https://github.com/taehyounpark/analogical/actions/workflows/macos.yml)
[![MIT License](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)

`queryosity` is a row-wise analysis description language for (semi-)structured data.

<p align="center">
	<strong> <a href="https://taehyounpark.github.io/queryosity/">Documentation</a></strong>
</p>

## Features

- Dataflow interface.
- Arbitrary data types.
- Lazy, multithreaded actions.
- Sensitivity analysis.

## Hello World
```cpp
#include <fstream>
#include <sstream>
#include <vector>

#include "queryosity.h"
#include "queryosity/hist.h"
#include "queryosity/json.h"

using dataflow = qty::dataflow;
namespace multithread = qty::multithread;
namespace dataset = qty::dataset;
namespace column = qty::column;
namespace query = qty::query;

using json = qty::json;
using h1d = qty::hist::hist<double>;
using linax = qty::hist::axis::regular;

int main() {
  dataflow df(multithread::enable(10));

  std::ifstream data("data.json");
  auto [x, v, w] = df.read(
      dataset::input<json>(data), dataset::column<double>("x"),
      dataset::column<std::vector<double>>("v"), dataset::column<double>("w"));

  auto zero = df.define(column::constant(0));
  auto v0 = v[zero];

  auto sel =
      df.weight(w)
          .filter(column::expression(
              [](std::vector<double> const &v) { return v.size(); }))(v)
          .filter(column::expression([](double x) { return x > 100.0; }))(x);

  auto h_x0_w = df.get(query::output<h1d>(linax(20, 0.0, 200.0)))
                    .fill(v0)
                    .at(sel)
                    .result();

  std::ostringstream os;
  os << *h_x0_w;
  std::cout << os.str() << std::endl;
}
```

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
cmake -DQUERYOSITY_INSTALL=ON ../
cmake --build .
cmake --install .
```
```cmake
find_package(queryosity 0.4.0 REQUIRED)
...
add_library(Analysis ...)
...
target_link_libraries(Analysis INTERFACE queryosity::queryosity)
```
```cpp
#include "queryosity.h"
```
#### Integrated
```cmake
add_subdirectory(queryosity)
...
add_library(Analysis ...)
...
target_link_libraries(Analysis INTERFACE queryosity::queryosity)
```
