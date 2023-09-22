# Home

![Version](https://img.shields.io/badge/Version-0.1.0-blue.svg)
[![Ubuntu](https://github.com/taehyounpark/analogical/actions/workflows/ubuntu.yml/badge.svg?branch=master)](https://github.com/taehyounpark/analogical/actions/workflows/ubuntu.yml)
[![macOS](https://github.com/taehyounpark/analogical/actions/workflows/macos.yml/badge.svg?branch=master)](https://github.com/taehyounpark/analogical/actions/workflows/macos.yml)
[![MIT License](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)

## Design goals

- **Coherent interface.** Higher-level languages have intuitive ways to do dataset transformations, e.g. the "DataFrame". The syntax used here aims to achieve the same level of abstraction in a unique way that was originally intended for high-energy physics use cases, referred to as a "DataFlow".
- **Custom implementations.** Users can implement any tabular data: a trivial example is used here, and the a realistic one of CERN ROOT Framework here. Similarly, custom columns and aggregations receive first-class treatment that supports arbitrary inputs, execution, and output.
- **Non-proliferative workflow.** Often times, small changes to an analysis need to be explored. How many times has CTRL+C/V been used to copy an entire analysis, made minute changes, and re-process the dataset? With built-in handling of "systematic variations", such changes can be performed and retrieved simultaneously.
- **Computational efficiency.** All operations within the dataset processing is performed at most once per-entry, only when needed. All systematic variations are processed at once. The dataset processing is multi-threaded for thread-safe plugins.