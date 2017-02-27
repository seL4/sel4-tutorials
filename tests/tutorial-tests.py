#!/usr/bin/env python
#
# Copyright 2016, NICTA
#
# This software may be distributed and modified according to the terms of
# the BSD 2-Clause license. Note that NO WARRANTY is provided.
# See "LICENSE_BSD2.txt" for details.
#
# @TAG(NICTA_BSD)
#

# Builds and runs all tutorial solutions, comparing output with expected
# completion text.

import sys, os, argparse, re, pexpect, subprocess, tempfile, logging
import xml.sax.saxutils

# this assumes this script is in a directory inside the tutorial directory
TUTORIAL_DIR = os.path.dirname(os.path.dirname(os.path.realpath(__file__)))
TOP_LEVEL_DIR = os.path.realpath(os.path.join(TUTORIAL_DIR, "..", ".."))

# timeout per test in seconds
DEFAULT_TIMEOUT = 1800

# Completion text for each test
COMPLETION = {
    "ia32_sel4_hello-1": "hello world",
    "ia32_sel4_hello-2": "(thread_2: hallo wereld)|(main: hello world)",
    "ia32_sel4_hello-3": "main: got a reply: 0xffff9e9e",
    "ia32_sel4_hello-4": "process_2: got a reply: 0xffff9e9e",
    "ia32_sel4_hello-2-nolibs": "(thread_2: hallo wereld)|(main: hello world)",
    "ia32_sel4_hello-3-nolibs": "main: got a reply: 0xffff9e9e",
    "ia32_sel4_hello-timer": "timer client wakes up: got the current timer tick:",
    "arm_camkes_hello-camkes-0": "Hello CAmkES World",
    "arm_camkes_hello-camkes-1": "Component echo saying: hello world",
    "arm_camkes_hello-camkes-2": "Caught cap fault in send phase at address 0x0",
    "arm_camkes_hello-camkes-timer": "After the client: wakeup",
    "ia32_camkes_hello-camkes-0": "Hello CAmkES World",
    "ia32_camkes_hello-camkes-1": "Component echo saying: hello world",
    "ia32_camkes_hello-camkes-2": "Caught cap fault in send phase at address 0x0"
}

# List of strings whose appearence in test output indicates test failure
FAILURE_TEXTS = [
    pexpect.EOF,
    pexpect.TIMEOUT,
    "Ignoring call to sys_exit_group"
]

ARCHITECTURES = ['arm', 'ia32']

def print_pexpect_failure(failure):
    if failure == pexpect.EOF:
        print("EOF received before completion text")
    elif failure == pexpect.TIMEOUT:
        print("Test timed out")

def app_names(arch, system):
    """
    Yields the names of all tutorial applications for a given architecture
    for a given system
    """

    build_config_dir = os.path.join(TUTORIAL_DIR, 'build-config')
    system_build_config_dir = os.path.join(build_config_dir, "configs-%s" % system)
    pattern = re.compile("^%s_(.*)_defconfig" % arch)
    for config in os.listdir(system_build_config_dir):
        matches = pattern.match(config)
        if matches is None:
            logging.info("Ignoring incompatible build config %s" % config)
        else:
            logging.info("Using build config %s" % config)
            app_name = matches.group(1)
            yield app_name

def arch_test_script(arch):
    return "%s/run-%s.py" % (TUTORIAL_DIR, arch)

def run_single_test(arch, system, app, timeout):
    """
    Builds and runs the solution to a given tutorial application for a given
    architecture for a given system, checking that the result matches the
    completion text
    """

    full_name = "%s_%s_%s" % (arch, system, app)
    try:
        completion_text = COMPLETION[full_name]
    except KeyError:
        logging.error("No completion text provided for %s." % full_name)
        sys.exit(1)

    # clean everything before each test
    make_mrproper = subprocess.Popen(['make', 'mrproper'], cwd=TOP_LEVEL_DIR)
    make_mrproper.wait()

    # run the test, storting output in a temporary file
    temp_file = tempfile.NamedTemporaryFile(delete=True)
    command = '%s %s' % (arch_test_script(arch), app)
    logging.info("Running command: %s" % command)
    test = pexpect.spawn(command, cwd=TOP_LEVEL_DIR)
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

    temp_file.close()

def run_arch_tests(arch, system, timeout):
    """
    Builds and runs all tests for a given architecture for a given system
    """

    logging.info("\nRunning %s tutorial tests for %s architecture..." % (system, arch))

    for app in app_names(arch, system):
        print("<testcase classname='sel4tutorials' name='%s_%s_%s'>" % (arch, system, app))
        run_single_test(arch, system, app, timeout)
        print("</testcase>")


def run_tests(system, timeout):
    """
    Builds and runs all tests for all architectures for a given system
    """

    print('<testsuite>')
    for arch in ARCHITECTURES:
        run_arch_tests(arch, system, timeout)
    print('</testsuite>')

def set_log_level(args):
    """
    Set the log level for the script from command line arguments
    """

    if args.verbose:
        logging.basicConfig(level=logging.DEBUG)
    elif args.quiet:
        logging.basicConfig(level=logging.ERROR)
    else:
        logging.basicConfig(level=logging.INFO)

def main():
    parser = argparse.ArgumentParser(
                description="Runs all tests for sel4 tutorials or camkes tutorials")

    parser.add_argument('system', choices=['sel4', 'camkes'],
                        help="which set of solutions to test")
    parser.add_argument('--verbose', action='store_true',
                        help="Output everything including debug info")
    parser.add_argument('--quiet', action='store_true',
                        help="Supress output except for junit xml")
    parser.add_argument('--timeout', type=int, default=DEFAULT_TIMEOUT)

    args = parser.parse_args()

    set_log_level(args)

    run_tests(args.system, args.timeout)

if __name__ == '__main__':
    sys.exit(main())
