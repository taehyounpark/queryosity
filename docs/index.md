# Queryosity

![Version](https://img.shields.io/badge/Version-0.2.0-blue.svg)
[![Ubuntu](https://github.com/taehyounpark/analogical/actions/workflows/ubuntu.yml/badge.svg?branch=master)](https://github.com/taehyounpark/analogical/actions/workflows/ubuntu.yml)
[![macOS](https://github.com/taehyounpark/analogical/actions/workflows/macos.yml/badge.svg?branch=master)](https://github.com/taehyounpark/analogical/actions/workflows/macos.yml)
[![Documentation](https://img.shields.io/badge/Documentation-mkdocs-blue.svg)](https://taehyounpark.github.io/analogical/home/design/)
[![MIT License](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)

`queryosity` is powerful data analysis library written in C++.

## Features

- *Dataflow* interface.
- Lazy actions.
- Multithreading.
- Sensitivity analysis.

## Design goals

`queryosity` is aims to achieve the following:

- **Clear interface.** Use a clear abstraction layer with modern C++ syntax to describe even the most complex analyses.
- **Customizable actions.** Support for custom datasets and queries, as well as arbitrary computations in-between.
- **Sensitivity analysis.** Systematic variations within an analysis are automatically propagated and simultaneously processed.
- **Computational efficiency.** Actions are performed for an entry only when required. Dataset traversal is multithreaded.

It does *not* strive to be well-suited for:

- Array-wise, i.e. columnar, analysis. Being **designed to handle arbitrary data types**, the dataset traversal is **inherently row-wise**. If an analysis consists mainly of bulk operations on arrays, then libraries with an index-based API (and SIMD support) will be better-suited (and faster).