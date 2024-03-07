# Queryosity

![Version](https://img.shields.io/badge/Version-0.2.0-blue.svg)
[![Ubuntu](https://github.com/taehyounpark/analogical/actions/workflows/ubuntu.yml/badge.svg?branch=master)](https://github.com/taehyounpark/analogical/actions/workflows/ubuntu.yml)
[![macOS](https://github.com/taehyounpark/analogical/actions/workflows/macos.yml/badge.svg?branch=master)](https://github.com/taehyounpark/analogical/actions/workflows/macos.yml)
[![Documentation](https://img.shields.io/badge/Documentation-mkdocs-blue.svg)](https://taehyounpark.github.io/analogical/home/design/)
[![MIT License](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)

`queryosity` is powerful row-wise data analysis library written in & for C++.

## Features

- **Clear interface.** The easy-to-learn, self-consistent API has a grand total of *five* main endpoints that can perform even the most complex operations.
- **Arbitrary data types.** Support columns of *any* data structure. Navigate linked lists, binary trees, etc.
- **Arbitrary actions.** The user, not the code, knows best. Provide customizable ABCs for not just columns but also input datasets and query outputs, leaving *all* details up to the implementation.
- **Lazy, but efficient.** An action is performed at most once per-entry only if needed. Traverse the dataset *once* to perform all actions at once.
- **Computational performance**. Statically compiled and multithreaded.
- **Systematic variations.** Perform *automatic* sensitivity analysis by propagating systematic variations through actions.
