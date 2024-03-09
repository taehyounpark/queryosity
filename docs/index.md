# Queryosity

![Version](https://img.shields.io/badge/Version-0.2.0-blue.svg)
[![Ubuntu](https://github.com/taehyounpark/analogical/actions/workflows/ubuntu.yml/badge.svg?branch=master)](https://github.com/taehyounpark/analogical/actions/workflows/ubuntu.yml)
[![macOS](https://github.com/taehyounpark/analogical/actions/workflows/macos.yml/badge.svg?branch=master)](https://github.com/taehyounpark/analogical/actions/workflows/macos.yml)
[![Documentation](https://img.shields.io/badge/Documentation-mkdocs-blue.svg)](https://taehyounpark.github.io/analogical/home/design/)
[![MIT License](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)

`queryosity` is powerful row-wise data analysis library written in & for C++.

## Features

- **Clear interface.** The easy-to-learn, self-consistent API has a grand total of *five* main endpoints that can perform even the most complex operations.
- **Arbitrary data types.** Manipulate columns of *any* data structure.
- **Arbitrary actions.** Provide ABCs to let users perform data analysis the way they want to, from input datasets to all the way to query outputs.
- **Lazy but efficient.** An action is performed for an entry only if needed. All actions are performed in one dataset traversal. Dataset traversal is multithreaded.
- **Systematic variations.** Perform *automatic* sensitivity analysis by propagating systematic variations through actions.
