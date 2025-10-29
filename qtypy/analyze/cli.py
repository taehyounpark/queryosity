import argparse
import sys
import re

def add_run_arguments(parser):
    parser.add_argument('analysis', type=str, help='Analysis to run (Python module path)')
    parser.add_argument('-i', '--files', nargs='+', required=True, help='Input file path(s)')
    parser.add_argument('-t', '--tree', default='events', help='Input tree name')
    parser.add_argument('-c', '--config', nargs='+', default=[], help='Configure analysis flags')
    parser.add_argument('-j', '--multithread', nargs='?', const=-1, default=0, type=int, help='Multithread the analysis')
    parser.add_argument('-n', '--entries', nargs='?', const=-1, default=-1, type=int, help='Number of entries to process')

def make_run_parser():
    parser = argparse.ArgumentParser(description='Run a queryosity analysis')
    add_run_arguments(parser)
    return parser

def parse_run_args():
    parser = make_run_parser()
    return parser.parse_args(sys.argv[1:])