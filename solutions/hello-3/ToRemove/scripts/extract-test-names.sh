#!/bin/bash
#
# Copyright 2014, NICTA
#
# This software may be distributed and modified according to the terms of
# the BSD 2-Clause license. Note that NO WARRANTY is provided.
# See "LICENSE_BSD2.txt" for details.
#
# @TAG(NICTA_BSD)
#

# Auto-generates list of tests from tests app for the test driver app to use.

if [ $# -ne 2 ]; then
    echo "Usage: $0 objdump-command target-image" 1>&2
    exit 1
fi

echo "#include <sel4test/test.h>"
echo ""; $1 -t -j _test_case $2 | grep -E " [lg][ ]+O _test_case.*TEST_" | tr -s ' ' | cut -d ' ' -f 5 | sort | while read line; do echo "__attribute__((used)) __attribute__((section(\"_test_case\"))) testcase_t ${line} = { .name = \"${line}\"};"; done; echo ""

