#!/usr/bin/env python3
#
# Copyright 2017, Data61, CSIRO (ABN 41 687 119 230)
#
# SPDX-License-Identifier: BSD-2-Clause
#

import os
import sys
import pexpect
import argparse

# This file contains a function that wraps a simulation call in a pexpect
# context and matches for completion or failure strings. Can also be templated
# by CMake to create a custom script that takes no arguments.


# List of strings whose appearence in test output indicates test failure
FAILURE_TEXTS = [
    pexpect.EOF,
    pexpect.TIMEOUT,
    "Ignoring call to sys_exit_group"
]


def simulate_with_checks(dir, completion_text, failure_list=FAILURE_TEXTS, logfile=sys.stdout):

    test = pexpect.spawnu("python3", args=["simulate"], cwd=dir)
    test.logfile = logfile
    for i in completion_text.split('\n') + ["\n"]:
        expect_strings = [i] + failure_list
        result = test.expect(expect_strings, timeout=10)

        # result is the index in the completion text list corresponding to the
        # text that was produced
        if result != 0:
            return result
    return 0


def main():
    finish_completion_text = """@FINISH_COMPLETION_TEXT@"""
    start_completion_text = """@START_COMPLETION_TEXT@"""
    parser = argparse.ArgumentParser(
        description="Initialize a build directory for completing a tutorial. Invoke from "
        "an empty sub directory, or the tutorials directory, in which case a "
        "new build directory will be created for you.")

    parser.add_argument('--text', default=finish_completion_text,
                        help="Output everything including debug info")
    parser.add_argument('--start', action='store_true',
                        help="Output everything including debug info")
    args = parser.parse_args()
    if args.start:
        completion_text = start_completion_text
    else:
        completion_text = args.text
    build_dir = os.path.dirname(__file__)
    result = simulate_with_checks(build_dir, completion_text)
    if result == 0:
        print("Success!")
    elif result <= len(FAILURE_TEXTS):
        print("Failure! {0}".format(FAILURE_TEXTS[result - 1]))
    else:
        print("Unknown reason for failure")

    return result


if __name__ == '__main__':
    sys.exit(main())
