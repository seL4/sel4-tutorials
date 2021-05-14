#!/usr/bin/env python3
#
# Copyright 201*, Data61, CSIRO (ABN 41 687 119 230)
#
# SPDX-License-Identifier: BSD-2-Clause
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
    parser.add_argument('--plat', type=str, choices=common.ALL_CONFIGS)
    parser.add_argument('--tut', type=str, choices=common.ALL_TUTORIALS, required=True)
    parser.add_argument('--solution', action='store_true',
                        help="Generate pre-made solutions", default=False)
    parser.add_argument('--task', help="Generate pre-made solutions")
    parser.add_argument('tutedir', nargs='?', default=os.getcwd())

    args = parser.parse_args()
    common.setup_logger(__name__)
    common.set_log_level(args.verbose, True)
    # Additional config/tutorial combination validation
    if not args.plat:
        # just pick the first platform that works for this tutorial
        args.plat = list(common.TUTORIALS[args.tut])[0]

    if args.plat not in common.TUTORIALS[args.tut]:
        logging.error("Tutorial %s not supported by platform %s. Valid platforms are %s: ",
                      args.tut, args.plat, common.TUTORIALS[args.tut])
        return -1
    # Check that the current working directory is empty. If not create a suitably
    # named build directory and switch to it
    dir_contents = os.listdir(args.tutedir)
    initialised = False
    if ".tute_config" in dir_contents:
        initialised = True
    if len(dir_contents) != 0 and ".tute_config" not in dir_contents:
        # Check that we are in the tutorial root directory before we decide to start
        # Making new directories
        if not os.access(os.getcwd() + "/init", os.X_OK):
            logging.error("Current dir %s is invalid" % os.getcwd())
            parser.print_help(sys.stderr)
            return -1
        tute_dir = os.path.join(os.getcwd(), args.tut)
        if os.path.exists(tute_dir):
            tute_dir = tempfile.mkdtemp(dir=os.getcwd(), prefix=('%s' % (args.tut)))
        else:
            os.mkdir(tute_dir)
        os.chdir(tute_dir)
    else:
        tute_dir = args.tutedir
    # Check that our parent directory is an expected tutorial root directory
    if not os.access(os.getcwd() + "/../init", os.X_OK):
        logging.error("Parent directory is not tutorials root directory")
        return -1
    # Initialize cmake. Output will be supressed as it defaults to the background
    build_dir = "%s_build" % tute_dir
    if not initialised:
        os.mkdir(build_dir)

    result = common.init_directories(args.plat, args.tut, args.solution,
                                     args.task, initialised, tute_dir, build_dir, sys.stdout)
    if result.exit_code != 0:
        logging.error("Failed to initialize build directory.")
        return -1
    # Inform the user about any subdirectory we might have made
    print("Tutorials created in subdirectory \"%s\"." % os.path.basename(tute_dir))
    print("Build directory initialised in \"%s\"." % os.path.basename(build_dir))
    return 0


if __name__ == '__main__':
    sys.exit(main())
