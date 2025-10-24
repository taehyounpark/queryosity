import os

def make_parser(subparsers):
    parser = subparsers.add_parser("submit", help="Submit an analysis job")

    from .cli import add_common_arguments
    add_common_arguments(parser)

    parser.add_argument("--name",
                        default=None,
                        type=str,
                        help="Job name (default: auto-generated from analysis name)")
    parser.add_argument("--split",
                        type=int,
                        default=1,
                        help="N-split of input files (default: 1)")
    parser.add_argument("--memory",
                        type=int,
                        default=4,
                        help="Memory limit in GB per job (default: 4)")
    parser.add_argument("--time",
                        type=int,
                        default=60,
                        help="Time limit in minutes per job (default: 60)")
    parser.add_argument("--disk",
                        type=int,
                        default=1000,
                        help="Disk space request in MB per job (default: 1000)")
    parser.add_argument("--dry",
                        action="store_true",
                        help="Print submission command without running it")

    return parser

def split_list(lst, n):
    """
    Split list `lst` into `n` approximately equal chunks.
    """
    k, m = divmod(len(lst), n)
    return [lst[i*k + min(i, m):(i+1)*k + min(i+1, m)] for i in range(n)]

def submit_analysis(args):

    cmd = ["analyze", args.analysis]
    if args.files:
        cmd += ["--files", *args.files]
    if args.config:
        cmd += ["--config", *args.config]
    print(f"Submitting job: {' '.join(cmd)}")

    # Split files across parts
    file_splits = split_list(args.files, args.split)
    print(f"Splitting {len(args.files)} input files into {args.split} split")

    for i, part_files in enumerate(file_splits, start=1):
        part_name = f"part{i}"

        # Build the command for this part
        cmd = ["analyze", args.analysis, "--files", *part_files]
        if args.config:
            cmd += ["--config", *args.config]
        cmd += [
            "--output", os.path.join(args.output, f"part{i}.root"),
            "--entries", str(args.entries),
        ]

    if not args.dry:
        pass