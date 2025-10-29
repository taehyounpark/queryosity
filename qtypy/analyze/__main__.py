import sys
from .cli import parse_run_args
from .run import run_analysis

def main():
    """Main entry point for the 'analyze' CLI."""
    args = parse_run_args()
    run_analysis(args)

if __name__ == "__main__":
    main()
