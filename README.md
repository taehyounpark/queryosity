# Queryosity

![Version](https://img.shields.io/badge/Version-0.3.2-blue.svg)
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
#include "queryosity/json.h"
#include "queryosity/hist.h"

#include "queryosity.h"

#include <fstream>
#include <vector>
#include <sstream>

using dataflow = qty::dataflow;
namespace multithread = qty::multithread;
namespace dataset = qty::dataset;
namespace column = qty::column;
namespace query = qty::query;

using json = qty::json;
using h1d = qty::hist::hist<double>;
using linax = qty::hist::axis::regular;

int main() {

	dataflow df( multithread::enable(10) );

	std::ifstream data("data.json");
	auto [x, w] = df.read( 
		dataset::input<json>(data), 
		dataset::column<std::vector<double>>("x"),
		dataset::column<double>("w") 
		);

	auto zero = df.define( column::constant(0) );
	auto x0 = x[zero];

	auto sel = df.weight(w).filter(
		column::expression([](std::vector<double> const& v){return v.size()}), x
		);

	auto h_x0_w = df.make( 
		query::plan<h1d>( linax(100,0.0,1.0) ) 
		).fill(x0).book(sel).result();

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
find_package(queryosity 0.3.2 REQUIRED)
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
