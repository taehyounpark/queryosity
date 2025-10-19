#!/usr/bin/env python3
import ROOT

import os
import re
import argparse
import importlib
import yaml

from qtypy import dataflow, dataset

def main():
    parser = argparse.ArgumentParser(description='Run a qtypy analysis')

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

    # ROOT.EnableImplicitMT(args.multithread)

    # load analysis module dynamically
    try:
        analysis = importlib.import_module(args.analysis)
    except ModuleNotFoundError:
        raise ImportError(f"Could not import analysis module '{args.analysis}'")

    # Set up dataflow & load dataset
    df = dataflow(multithreaded = args.multithread > 0, n_threads=args.multithread, n_rows = args.entries)
    df.load(dataset.tree(file_paths=args.files, tree_name=args.tree))

    # load config files into flags
    flags = {}
    for cfg_file_path in args.config:
        with open(cfg_file_path, "r") as f:  # YAML files are text, not binary
            kvs = yaml.safe_load(f)
        flags.update(kvs)

    # run user-defined analysis
    if hasattr(analysis, "analyze"):
        analysis.analyze(df, flags)
    else:
        raise AttributeError(
            f"'{args.analysis}' module must define an 'analyze(df, flags)' function"
        )

    # execute analysis
    results = df.analyze()

    # handle output file
    output_file = ROOT.TFile(args.output, "RECREATE" if args.force else "CREATE")

    # Write output via analysis module
    if hasattr(analysis, "output"):
        analysis.output(output_file, results)
    else:
        raise AttributeError(
            f"'{args.analysis}' module must define an 'output(df, results)' function"
        )

    del output_file

if __name__ == "__main__":
    main()