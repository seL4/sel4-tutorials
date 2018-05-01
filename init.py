#!/usr/bin/env python
#
# Copyright 201*, Data61
# Commonwealth Scientific and Industrial Research Organisation (CSIRO)
# ABN 41 687 119 230.
#
# This software may be distributed and modified according to the terms of
# the BSD 2-Clause license. Note that NO WARRANTY is provided.
# See "LICENSE_BSD2.txt" for details.
#
# @TAG(DATA61_BSD)
#

# Wrapper script to setup a build directory ready for doing the tutorials

import common
import sys
import argparse
import logging
import os
import sh
import tempfile

def main():
    parser = argparse.ArgumentParser(
                description="Initialize a build directory for completing a tutorial. Invoke from "
                            "an empty sub directory, or the tutorials directory, in which case a "
                            "new build directory will be created for you.")

    parser.add_argument('--verbose', action='store_true',
                        help="Output everything including debug info", default=False)
    parser.add_argument('--quiet', action='store_true',
                        help="Suppress output except for tutorial completion information", default=True)
    parser.add_argument('--plat', type=str, choices=common.ALL_CONFIGS, required=True)
    parser.add_argument('--tut', type=str, choices=common.ALL_TUTORIALS, required=True)
    parser.add_argument('--solution', action='store_true', help="Generate pre-made solutions", default=False)
    args = parser.parse_args()
    common.setup_logger(__name__)
    common.set_log_level(args.verbose, True)
    # Additional config/tutorial combination validation
    if args.plat not in common.TUTORIALS[args.tut]:
        logging.error("Tutorial %s not supported by platform %s. Valid platforms are %s: ", args.tut, args.plat, common.TUTORIALS[args.tut])
        return -1
    # Check that the current working directory is empty. If not create a suitably
    # named build directory and switch to it
    if len(os.listdir(os.getcwd())) != 0:
        # Check that we are in the tutorial root directory before we decide to start
        # Making new directories
        if not os.access(os.getcwd() + "/init", os.X_OK):
            logging.error("Current dir %s is invalid" % os.getcwd())
            parser.print_help(sys.stderr)
            return -1
        build_dir = tempfile.mkdtemp(dir=os.getcwd(), prefix=('build_%s_%s' % (args.plat, args.tut)))
        os.chdir(build_dir)
    else:
        build_dir = None
    # Check that our parent directory is an expected tutorial root directory
    if not os.access(os.getcwd() + "/../init", os.X_OK):
        logging.error("Parent directory is not tutorials root directory")
        return -1
    # Initialize cmake. Output will be supressed as it defaults to the background
    result = common.init_build_directory(args.plat, args.tut, args.solution, os.getcwd())
    if result.exit_code != 0:
        logging.error("Failed to initialize build directory.")
        return -1
    # Run cmake again but show the tutorial information (to do this we run it in the foreground)
    sh.cmake(['-DTUTORIALS_PRINT_INSTRUCTIONS=TRUE', '..'], _fg=True)
    # Run again in the background but turn of printing
    sh.cmake(['-DTUTORIALS_PRINT_INSTRUCTIONS=FALSE', '..'])
    # Inform the user about any subdirectory we might have made
    if build_dir is not None:
        print("Tutorials created in subdirectory \"%s\". Switch to this directory and follow the instructions above to complete." % os.path.basename(build_dir))
    return 0

if __name__ == '__main__':
    sys.exit(main())
