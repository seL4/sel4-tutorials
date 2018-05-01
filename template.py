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

import jinja2
import argparse
import sys
import sh

def main():
    parser = argparse.ArgumentParser(description='Tutorial script template parser. Template is read from '
        'stdin and outout is placed in stdout')
    parser.add_argument('-s','--solution', action='store_true', default=False)
    args = parser.parse_args()
    input = sys.stdin.read()
    template = jinja2.Template(input,
        block_start_string='/*-',
        block_end_string='-*/'
    )
        
    sys.stdout.write(template.render(solution=args.solution))
    return 0


if __name__ == '__main__':
    sys.exit(main())
