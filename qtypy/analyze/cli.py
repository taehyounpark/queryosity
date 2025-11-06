import argparse
import sys
import re

def add_run_arguments(parser):

    parser.add_argument(
        "analysis",
        type=str,
        help="Python module path of the analysis to run."
    )

    parser.add_argument(
        "-i", "--files",
        nargs="+",
        required=True,
        help="Input file(s)."
    )

    parser.add_argument(
        "-t", "--tree",
        default="events",
        help="Name of the input tree (default: 'events')."
    )

    parser.add_argument(
        "-c", "--config",
        nargs="+",
        default=[],
        help="Configuration flags for the analysis."
    )

    parser.add_argument(
        "-j", "--multithread",
        nargs="?",
        const=-1,
        default=0,
        type=int,
        help="Enable multithreading. Specify number of threads, -1 for all cores, or 0 to disable (default: 0)."
    )

    parser.add_argument(
        "-n", "--entries",
        nargs="?",
        const=-1,
        default=-1,
        type=int,
        help="Number of entries to process (-1 for all; default: -1)."
    )

    parser.add_argument(
        "-o", "--output",
        type=str,
        default=None,
        help="Output path (file or directory)."
    )

def make_run_parser():
    parser = argparse.ArgumentParser(description='Run a queryosity analysis')
    add_run_arguments(parser)
    return parser

def parse_run_args():
    parser = make_run_parser()
    return parser.parse_args(sys.argv[1:])