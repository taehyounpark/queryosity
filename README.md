# Queryosity

![Version](https://img.shields.io/badge/Version-0.5.0-blue.svg)
![C++ Standard](https://img.shields.io/badge/C++-17-blue.svg)
[![Ubuntu](https://github.com/taehyounpark/analogical/actions/workflows/ubuntu.yml/badge.svg?branch=master)](https://github.com/taehyounpark/analogical/actions/workflows/ubuntu.yml)
[![macOS](https://github.com/taehyounpark/analogical/actions/workflows/macos.yml/badge.svg?branch=master)](https://github.com/taehyounpark/analogical/actions/workflows/macos.yml)
[![MIT License](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)


Queryosity is a row-wise data analysis library for (semi-)structured data.

- Dataflow interface.
- Arbitrary data types.
- Lazy, multithreaded actions.
- Sensitivity analysis.

<p align="center">
	<strong> <a href="https://queryosity.readthedocs.io/">Documentation</a></strong>
</p>

## Hello World
```cpp
#include <fstream>
#include <sstream>
#include <vector>

#include <queryosity.hpp>
#include <queryosity/hist.hpp>
#include <queryosity/json.hpp>

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
  auto [x, w] = df.read(
      dataset::input<json>(data), dataset::column<std::vector<double>>("x"), dataset::column<double>("w"));

  auto zero = df.define(column::constant(0));
  auto x0 = x[zero];

  auto sel =
      df.weight(w)
          .filter(column::expression(
              [](std::vector<double> const &v) { return v.size(); }))(x);

  auto h_x0_w = df.get(query::output<h1d>(linax(20, 0.0, 200.0)))
                    .fill(x0)
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
#include <queryosity.hpp>
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
find_package(queryosity 0.5.0 REQUIRED)
...
add_library(Analysis ...)
...
target_link_libraries(Analysis INTERFACE queryosity::queryosity)
```
```cpp
#include <queryosity.hpp>
```
#### Integrated
```cmake
add_subdirectory(queryosity)
...
add_library(Analysis ...)
...
target_link_libraries(Analysis INTERFACE queryosity::queryosity)
```
