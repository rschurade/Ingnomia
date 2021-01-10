#!/usr/bin/env python3

import argparse
import os
import sys

from lib.util import DOCDIR
from lib.render import render

if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("-t", "--theme", help="theme name, defaults to 'default'", default="default")
    parser.add_argument(
        "-o",
        "--output",
        help="output directory, defaults to 'docs/html' in the repository",
        default=os.path.join(DOCDIR, "html"),
    )
    parser.add_argument(
        "-f", "--overwrite", help="allow overwriting output directory when it exists", action="store_true"
    )
    parser.add_argument("-b", "--build", help="set build ID information", default=None)

    ns = parser.parse_args()

    if os.path.exists(ns.output) and not ns.overwrite:
        print(f"Error: output path {ns.output} exists, pass --overwrite to overwrite")
        sys.exit(1)

    render(ns.theme, ns.output, ns.build)
