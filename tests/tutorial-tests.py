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

import sys, os, argparse, re, pexpect, subprocess, tempfile

# this assumes this script is in a directory inside the tutorial directory
TUTORIAL_DIR = os.path.dirname(os.path.dirname(os.path.realpath(__file__)))
TOP_LEVEL_DIR = os.path.realpath(os.path.join(TUTORIAL_DIR, "..", ".."))

# timeout per test in seconds
TIMEOUT = 1800

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
FAILURE_TEXT = [
    pexpect.EOF,
    "Ignoring call to sys_exit_group"
]

ARCHITECTURES = ['arm', 'ia32']

def escape_xml(string):
    return string.replace('<', '&lt;').replace('>', '&gt;')

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
        if matches is not None:
            app_name = matches.group(1)
            yield app_name

def arch_test_script(arch):
    return "%s/run-%s.sh" % (TUTORIAL_DIR, arch)

def run_single_test(arch, system, app):
    """
    Builds and runs the solution to a given tutorial application for a given
    architecture for a given system, checking that the result matches the
    completion text
    """

    full_name = "%s_%s_%s" % (arch, system, app)
    completion_text = COMPLETION[full_name]

    # clean everything before each test
    make_mrproper = subprocess.Popen(['make', 'mrproper'], cwd=TOP_LEVEL_DIR)
    make_mrproper.wait()

    # run the test, storting output in a temporary file
    temp_file = tempfile.NamedTemporaryFile(delete=True)
    command = '%s %s' % (arch_test_script(arch), app)
    test = pexpect.spawn(command)
    test.logfile = temp_file
    result = test.expect([completion_text] + FAILURE_TEXT, timeout=TIMEOUT)

    # result is the index in the completion text list corresponding to the
    # text that was produced
    if result == 0:
        print("Success!")
    else:
        print("<failure type='failure'>")
        # print the log file's contents to help debug the failure
        temp_file.seek(0)
        print(escape_xml(temp_file.read()))
        print("</failure>")

def run_arch_tests(arch, system):
    """
    Builds and runs all tests for a given architecture for a given system
    """

    for app in app_names(arch, system):
        print("<testcase classname='sel4tutorials' name='%s_%s_%s'>" % (arch, system, app))
        run_single_test(arch, system, app)
        print("</testcase>")


def run_tests(system):
    """
    Builds and runs all tests for all architectures for a given system
    """

    print('<testsuite>')
    for arch in ARCHITECTURES:
        run_arch_tests(arch, system)
    print('</testsuite>')

def main():
    parser = argparse.ArgumentParser(
                description="Runs all tests for sel4 tutorials or camkes tutorials")

    parser.add_argument('system', choices=['sel4', 'camkes'],
                        help="which set of solutions to test")

    args = parser.parse_args()
    run_tests(args.system)

if __name__ == '__main__':
    sys.exit(main())
