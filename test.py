#!/usr/bin/env python3
#
# Copyright 2017, Data61, CSIRO (ABN 41 687 119 230)
#
# SPDX-License-Identifier: BSD-2-Clause
#

# Builds and runs all tutorial solutions, comparing output with expected
# completion text.

import sys
import os
import argparse
import re
import pexpect
import subprocess
import tempfile
import logging
import signal
import psutil
import shutil
import os.path
import xml.sax.saxutils
import sh
import tools
from tools import expect
import common

# this assumes this script is in a directory inside the tutorial directory
TUTORIAL_DIR = common.get_tutorial_dir()
TOP_LEVEL_DIR = common.get_project_root()


def print_pexpect_failure(failure):
    if failure == pexpect.EOF:
        print("EOF received before completion text")
    elif failure == pexpect.TIMEOUT:
        print("Test timed out")


def run_single_test_iteration(build_dir, solution, logfile):
    # Build
    result = sh.ninja(_out=logfile, _cwd=build_dir)
    if result.exit_code != 0:
        logging.error("Failed to build. Not deleting build directory %s" % build_dir)
        sys.exit(1)

    check = sh.Command(os.path.join(build_dir, "check"))
    if solution:
        result = check(_out=logfile, _cwd=build_dir)
    else:
        # We check the start state if not solution
        result = check("--start", _out=logfile, _cwd=build_dir)
    for proc in psutil.process_iter():
        if "qemu" in proc.name():
            proc.kill()
    return result.exit_code


def run_single_test(config, tutorial, temp_file):
    """
    Builds and runs the solution to a given tutorial application for a given
    configuration, checking that the result matches the completion text
    """
    # Create temporary directory for working in (make this a common helper to share with init.py)
    tute_dir = tempfile.mkdtemp(dir=TOP_LEVEL_DIR, prefix=tutorial)
    build_dir = "%s_build" % tute_dir
    os.mkdir(build_dir)

    # Initialize directories
    result = common.init_directories(config, tutorial, False, None,
                                     False, tute_dir, build_dir, temp_file)
    if result.exit_code != 0:
        logging.error("Failed to initialize tute directory. Not deleting tute directory %s" % build_dir)
        sys.exit(1)

    tasks = open(os.path.join(tute_dir, ".tasks"), 'r').read()
    for task in tasks.strip().split('\n'):
        for solution in [False, True]:
            print("Testing: task: %s solution: %s" % (task, "True" if solution else "False"))
            result = common.init_directories(
                config, tutorial, solution, task, True, tute_dir, build_dir, temp_file)
            if result.exit_code != 0:
                logging.error(
                    "Failed to initialize tute directory. Not deleting tute directory %s" % build_dir)
                sys.exit(1)
            if run_single_test_iteration(build_dir, solution, temp_file):
                print("<failure type='failure'>")
                return 1
            else:
                logging.info("Success!")

    shutil.rmtree(build_dir)
    shutil.rmtree(tute_dir)


def run_tests(tests):
    """
    Builds and runs a list of tests
    """

    print('<testsuite>')
    for (config, app) in tests:
        print("<testcase classname='sel4tutorials' name='%s_%s'>" % (config, app))
        temp_file = tempfile.NamedTemporaryFile(delete=True, mode='w+', encoding='utf-8')
        try:
            run_single_test(config, app, temp_file)
        except:
            temp_file.seek(0)
            print(temp_file.read())
            raise
    print('</testsuite>')


def main():
    parser = argparse.ArgumentParser(
        description="Runs all tests for the tutorials")

    parser.add_argument('--verbose', action='store_true',
                        help="Output everything including debug info")
    parser.add_argument('--quiet', action='store_true',
                        help="Suppress output except for junit xml")
    parser.add_argument('--config', type=str, choices=common.ALL_CONFIGS)
    parser.add_argument('--app', type=str, choices=common.ALL_TUTORIALS)

    args = parser.parse_args()

    common.setup_logger(__name__)
    common.set_log_level(args.verbose, args.quiet)

    # Generate all the tests (restricted by app and config)
    tests = []
    for tutorial in common.ALL_TUTORIALS:
        for config in common.TUTORIALS[tutorial]:
            if (args.app is None or args.app == tutorial) and \
               (args.config is None or args.config == config):
                tests = tests + [(config, tutorial)]

    run_tests(tests)
    return 0


if __name__ == '__main__':
    sys.exit(main())
