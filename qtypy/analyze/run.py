import ROOT

from qtypy import dataflow, dataset

import os
import importlib
import yaml

def make_parser(subparsers):
    parser = subparsers.add_parser("run", help="Run analysis locally")
    from .cli import add_common_arguments
    add_common_arguments(parser)
    return parser

def run_analysis(args):

    # load analysis module dynamically
    try:
        analysis = importlib.import_module(args.analysis)
    except ModuleNotFoundError:
        raise ImportError(f"Could not import analysis module '{args.analysis}'")

    # Set up dataflow & load dataset
    df = dataflow(multithreaded = args.multithread > 0, n_threads=args.multithread, n_rows = args.entries)
    df << {args.tree : dataset.tree(file_paths=args.files, tree_name=args.tree)}

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