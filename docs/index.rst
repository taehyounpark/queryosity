.. ana documentation master file, created by
   sphinx-quickstart on Wed Apr 24 15:19:01 2019.
   You can adapt this file completely to your liking, but it should at least
   contain the root `toctree` directive.

Coherent data analysis in C++

Introduction
============

The purpose of ``ana`` is to provide a clear *abstraction* layer for dataset transformation procedures.

- A ``dataflow`` entity defines the set of operations on the dataset.
- An operation is a ``lazy`` action to be performed in a row-wise manner.
- An action can be ``varied``, meaning alternate versions are performed at once.

Features
========

* Manipulation of any data types as column values.
* Arbitrary action execution and results retrieval.
* Propagation of systematic variations.
* Multithreaded processing of the dataset.

.. toctree::
   :caption: INSTALLATION
   :maxdepth: 1

   installation/cmake

.. toctree::
   :caption: HOW TO USE
   :maxdepth: 1

   usage/quickstart
   usage/systematic_variations

.. toctree::
   :caption: API REFERENCE
   :maxdepth: 1

   api/datasets
   api/actions
   api/analysis