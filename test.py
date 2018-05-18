#!/usr/bin/env python
#
# Copyright 2017, Data61
# Commonwealth Scientific and Industrial Research Organisation (CSIRO)
# ABN 41 687 119 230.
#
# This software may be distributed and modified according to the terms of
# the BSD 2-Clause license. Note that NO WARRANTY is provided.
# See "LICENSE_BSD2.txt" for details.
#
# @TAG(DATA61_BSD)
#

# Builds and runs all tutorial solutions, comparing output with expected
# completion text.

import sys, os, argparse, re, pexpect, subprocess, tempfile, logging
import signal
import psutil
import shutil
import os.path
import xml.sax.saxutils
import sh

import common

# this assumes this script is in a directory inside the tutorial directory
TUTORIAL_DIR = common.get_tutorial_dir()
TOP_LEVEL_DIR = common.get_project_root()

# timeout per test in seconds
DEFAULT_TIMEOUT = 1800

# Completion text for each test
COMPLETION = {
    "hello-1": "hello world",
    "hello-2": "(thread_2: hallo wereld)|(main: hello world)",
    "hello-2-nolibs": "(thread_2: hallo wereld)|(main: hello world)",
    "hello-3": "main: got a reply: 0xffff*9e9e",
    "hello-3-nolibs": "main: got a reply: 0xffff*9e9e",
    "hello-4": "process_2: got a reply: 0xffff*9e9e",
    "hello-timer": "timer client wakes up: got the current timer tick:",
    "hello-camkes-0": "Hello CAmkES World",
    "hello-camkes-1": "Component echo saying: hello world",
    "hello-camkes-2": "FAULT HANDLER: data fault from client.control",
    "hello-camkes-timer": "After the client: wakeup",
}

# List of strings whose appearence in test output indicates test failure
FAILURE_TEXTS = [
    pexpect.EOF,
    pexpect.TIMEOUT,
    "Ignoring call to sys_exit_group"
]

def print_pexpect_failure(failure):
    if failure == pexpect.EOF:
        print("EOF received before completion text")
    elif failure == pexpect.TIMEOUT:
        print("Test timed out")

def run_single_test(config, tutorial, timeout):
    """
    Builds and runs the solution to a given tutorial application for a given
    configuration, checking that the result matches the completion text
    """

    try:
        completion_text = COMPLETION[tutorial]
    except KeyError:
        logging.error("No completion text provided for %s." % tutorial)
        sys.exit(1)

    # Create temporary directory for working in (make this a common helper to share with init.py)
    build_dir = tempfile.mkdtemp(dir=TOP_LEVEL_DIR, prefix='build_')

    # Initialize build directory (check results?)
    result = common.init_build_directory(config, tutorial, True, build_dir, output=sys.stdout)
    if result.exit_code != 0:
        logging.error("Failed to initialize build directory. Not deleting build directory %s" % build_dir)
        sys.exit(1)

    # Build
    result = sh.ninja(_out=sys.stdout, _cwd=build_dir)
    if result.exit_code != 0:
        logging.error("Failed to build. Not deleting build directory %s" % build_dir)
        sys.exit(1)

    # run the test, storting output in a temporary file
    temp_file = tempfile.NamedTemporaryFile(delete=True)
    logging.info("Running ./simulate")
    test = pexpect.spawn("sh", args=["simulate"], cwd=build_dir)
    test.logfile = temp_file
    expect_strings = [completion_text] + FAILURE_TEXTS
    result = test.expect(expect_strings, timeout=timeout)

    # result is the index in the completion text list corresponding to the
    # text that was produced
    if result == 0:
        logging.info("Success!")
    else:
        print("<failure type='failure'>")
        # print the log file's contents to help debug the failure
        temp_file.seek(0)
        print(xml.sax.saxutils.escape(temp_file.read()))
        print_pexpect_failure(expect_strings[result])
        print("</failure>")

    for proc in psutil.process_iter():
        if "qemu" in proc.name():
            proc.kill()
    temp_file.close()
    shutil.rmtree(build_dir)

def run_tests(tests, timeout):
    """
    Builds and runs a list of tests
    """

    print('<testsuite>')
    for (config, app) in tests:
        print("<testcase classname='sel4tutorials' name='%s_%s'>" % (config,app))
        run_single_test(config, app, timeout)
    print('</testsuite>')

def main():
    parser = argparse.ArgumentParser(
                description="Runs all tests for the tutorials")

    parser.add_argument('--verbose', action='store_true',
                        help="Output everything including debug info")
    parser.add_argument('--quiet', action='store_true',
                        help="Suppress output except for junit xml")
    parser.add_argument('--timeout', type=int, default=DEFAULT_TIMEOUT)
    parser.add_argument('--config', type=str, choices=common.ALL_CONFIGS)
    parser.add_argument('--app', type=str, choices=common.ALL_TUTORIALS)

    args = parser.parse_args()

    common.setup_logger(__name__)
    common.set_log_level(args.verbose, args.quiet)

    # Generate all the tests (restricted by app and config)
    tests=[]
    for tutorial in common.ALL_TUTORIALS:
        for config in common.TUTORIALS[tutorial]:
            if (args.app is None or args.app == tutorial) and \
               (args.config is None or args.config == config):
                tests = tests + [(config, tutorial)]

    run_tests(tests, args.timeout)
    return 0

if __name__ == '__main__':
    sys.exit(main())
