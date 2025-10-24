import sys
from .cli import parse_args
from .run import run_analysis
from .submit import submit_analysis

def main():
    """Main entry point for the 'analyze' CLI."""
    args = parse_args()

    if args.command == "run":
        run_analysis(args)
    elif args.command == "submit":
        submit_analysis(args)
    else:
        # No command given → show help
        print("Error: no command specified.\n")
        from .cli import make_top_parser
        make_top_parser().print_help()
        sys.exit(1)

if __name__ == "__main__":
    main()
