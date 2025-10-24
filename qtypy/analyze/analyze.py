#!/usr/bin/env python3
import ROOT

import os
import re
import argparse
import importlib
import yaml

from qtypy import dataflow, dataset

def main():
    parser = argparse.ArgumentParser(description='Perform a queryosity analysis')

    def py_module(value):
        if not re.match(r'^[a-zA-Z_][a-zA-Z0-9_\.]*$', value):
            raise argparse.ArgumentTypeError(f"Invalid module name: {value}")
        return value

    parser.add_argument(
        'analysis',
        type=py_module,
        help='Analysis to run (Python module path)'
    )

    parser.add_argument('--files', nargs='+', required=True,
                        help='Input file path(s)')
    parser.add_argument('--tree', default='events',
                        help='Input tree name')

    parser.add_argument('--config', '-c', nargs='+', default=[],
                        help='Configure analysis flags')

    parser.add_argument(
        '-j', '--multithread',
        nargs='?', const=-1, default=0, type=int,
        help='Multithread the analysis'
    )

    parser.add_argument(
        '-n', '--entries',
        nargs='?', const=-1, default=-1, type=int,
        help='Number of entries to process'
    )

    parser.add_argument('--output', default='analyzed.root',
                        help='Output file name')
    parser.add_argument('--force', action='store_true',
                        help='Force overwrite the output')

    args = parser.parse_args()


if __name__ == "__main__":
    main()