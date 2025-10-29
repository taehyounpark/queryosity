import argparse
import sys
import re

def add_common_arguments(parser):
    parser.add_argument('analysis', type=str, help='Analysis to run (Python module path)')
    parser.add_argument('-i', '--files', nargs='+', required=True, help='Input file path(s)')
    parser.add_argument('-t', '--tree', default='events', help='Input tree name')
    parser.add_argument('-c', '--config', nargs='+', default=[], help='Configure analysis flags')
    parser.add_argument('-j', '--multithread', nargs='?', const=-1, default=0, type=int, help='Multithread the analysis')
    parser.add_argument('-n', '--entries', nargs='?', const=-1, default=-1, type=int, help='Number of entries to process')

def make_top_parser():
    parser = argparse.ArgumentParser(description='Perform a queryosity analysis')
    subparsers = parser.add_subparsers(dest="command")

    from . import run, submit
    run.make_parser(subparsers)
    submit.make_parser(subparsers)
    return parser

def parse_args():
    parser = make_top_parser()
    argv = sys.argv[1:]

    # if no subcommand given but first arg isn't a known one, default to 'run'
    if len(argv) > 0 and argv[0] not in {"jit", "run", "submit"}:
        argv = ["run"] + argv

    return parser.parse_args(argv)
