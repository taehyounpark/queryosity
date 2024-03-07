# Queryosity

![Version](https://img.shields.io/badge/Version-0.2.0-blue.svg)
[![Ubuntu](https://github.com/taehyounpark/analogical/actions/workflows/ubuntu.yml/badge.svg?branch=master)](https://github.com/taehyounpark/analogical/actions/workflows/ubuntu.yml)
[![macOS](https://github.com/taehyounpark/analogical/actions/workflows/macos.yml/badge.svg?branch=master)](https://github.com/taehyounpark/analogical/actions/workflows/macos.yml)
[![Documentation](https://img.shields.io/badge/Documentation-mkdocs-blue.svg)](https://taehyounpark.github.io/analogical/home/design/)
[![MIT License](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)

`queryosity` is powerful row-wise data analysis library written in & for C++.

## Features

- **Clear interface.** The easy-to-learn, self-consistent API has a grand total of *five* main endpoints that can perform even the most complex operations.
- **Arbitrary data types.** Support columns of *any* data structure. Linked lists? Nested trees? No problem.
- **Arbitrary actions.** Provide customizable ABCs for not just columns, but also input datasets and query outputs. Leave *all* details up to the analyzer as desired.
- **Lazy, but efficient.** An action is performed at most once per-entry only if needed. Traverse the dataset *once* to perform all actions.
- **Computational performance**. Statically compile the entire analysis. Use multithreading.
- **Systematic variations.** Perform sensitivity analysis by specifying systematic variations that are automatically propagated and simultaneously processed.
