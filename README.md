# Queryosity

![Version](https://img.shields.io/badge/Version-0.2.0-blue.svg)
[![Ubuntu](https://github.com/taehyounpark/analogical/actions/workflows/ubuntu.yml/badge.svg?branch=master)](https://github.com/taehyounpark/analogical/actions/workflows/ubuntu.yml)
[![macOS](https://github.com/taehyounpark/analogical/actions/workflows/macos.yml/badge.svg?branch=master)](https://github.com/taehyounpark/analogical/actions/workflows/macos.yml)
[![Documentation](https://img.shields.io/badge/Documentation-mkdocs-blue.svg)](https://taehyounpark.github.io/analogical/home/design/)
[![MIT License](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)

`queryosity` is row-wise data analysis library written in & for C++.

<p align="center">
	<strong> <a href="https://taehyounpark.github.io/queryosity/">See full documentation</a></strong>
</p>

## Features

- **Clear interface.** The easy-to-learn, self-consistent API has a grand total of *five* main endpoints that can perform even the most complex operations.
- **Arbitrary data types.** Manipulate columns of *any* data structure.
- **Arbitrary actions.** Provide ABCs to let users perform data analysis the way they want to, from input datasets to all the way to query outputs.
- **Lazy but efficient.** An action is performed for an entry only if needed. All actions are performed in one dataset traversal. Dataset traversal is multithreaded.
- **Systematic variations.** Perform *automatic* sensitivity analysis by propagating systematic variations through actions.

## Hello World
```cpp
#include "queryosity/json.h"
#include "queryosity/hist.h"

#include "queryosity.h"

#include <fstream>
#include <vector>
#include <sstream>

namespace qty = queryosity;

using dataflow = qty::dataflow;
namespace multithread = qty::multithread;
namespace dataset = qty::dataset;
namespace column = qty::column;
namespace query = qty::query;

using json = qty::json;
using hist_1d = qty::hist::hist<double>;
using lin_ax = qty::hist::axis::linear;

int main() {

	dataflow df( multithread::enable(10) );

	std::ifstream data("data.json");
	auto [x, w] = df.read( 
		dataset::input<json>(data), 
		dataset::columns<std::vector<double>, double>("x", "w") 
		);

	auto zero = df.define( column::constant(0) );
	auto x0 = x[zero];

	auto sel = df.weight(w).filter(
		column::expression([](std::vector<double> const& v){return v.size()}), x
		);

	auto h_x0_w = df.make( 
		query::plan<hist_1d>( lin_ax(100,0.0,1.0) ) 
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
