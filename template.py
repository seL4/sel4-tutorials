#!/usr/bin/env python
#
# Copyright 2018, Data61
# Commonwealth Scientific and Industrial Research Organisation (CSIRO)
# ABN 41 687 119 230.
#
# This software may be distributed and modified according to the terms of
# the BSD 2-Clause license. Note that NO WARRANTY is provided.
# See "LICENSE_BSD2.txt" for details.
#
# @TAG(DATA61_BSD)
#
from __future__ import print_function
import os
import jinja2
import argparse
import sys
import sh
from yaml import load, dump
try:
    from yaml import CLoader as Loader, CDumper as Dumper
except ImportError:
    from yaml import Loader, Dumper

def main():
    parser = argparse.ArgumentParser(description='Tutorial script template parser. Template is read from '
        'stdin and outout is placed in stdout')
    parser.add_argument('-s','--solution', action='store_true', default=False)
    parser.add_argument('--tut-file')
    parser.add_argument('--out-dir')
    parser.add_argument('--input-files', type=argparse.FileType('w'))
    parser.add_argument('--output-files', type=argparse.FileType('w'))
    args = parser.parse_args()

    # Save this script to input deps file
    print(os.path.realpath(__file__), file=args.input_files)

    # Read list of files to generate into dict
    data = {}
    with open(args.tut_file, 'r') as stream:
        data = load(stream, Loader=Loader)
        # Save yaml file to input deps file
        print(stream.name, file=args.input_files)

    for file in data['render']:
        filename = os.path.join(args.out_dir, file)

        # Create required directories
        if not os.path.exists(os.path.dirname(filename)):
            os.makedirs(os.path.dirname(filename))

        with open(os.path.join(os.path.split(args.tut_file)[0], file), 'r') as in_stream, \
          open(filename, 'w') as out_stream:

            # Save dependencies to deps files
            print(in_stream.name, file=args.input_files)
            print(out_stream.name, file=args.output_files)

            # process template file
            input = in_stream.read()
            template = jinja2.Template(input,
                block_start_string='/*-',
                block_end_string='-*/'
            )

            out_stream.write(template.render(solution=args.solution))

    return 0


if __name__ == '__main__':
    sys.exit(main())
