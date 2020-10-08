#!/usr/bin/env python

import os.path
from argparse import ArgumentParser

__version__ = "1.0"

def main():

    parser = ArgumentParser(description="Binary to C header converter")
    parser.add_argument("--version", action="version", version="%(prog)s " + __version__)
    parser.add_argument("file", help="the file to be converted")

    args = parser.parse_args()

    basename = os.path.basename(args.file).replace(' ', '_').replace('-', '_').replace('.', '_')
    
    with open(args.file, "rb") as fd:
        data = bytearray(fd.read())

    print("/* %s, %d bytes */\n" % (args.file, len(data)))
    print("const uint8_t %s[] =\n{" % basename)
    line = "" 

    for i in data:
        num = "%d" % i + ","
        if len(line) + len(num) < 100:
            line += num
        else:
            print("    " + line)
            line = num

    print("    " + line[:-1])
    print("};\n")

if __name__ == "__main__":
    main()
