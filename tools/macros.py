#
# Copyright 2017, Data61, CSIRO (ABN 41 687 119 230)
#
# SPDX-License-Identifier: BSD-2-Clause
#

from .tutorialstate import TaskContentType


def ninja_block():
    """Print ninja code block"""
    return '''
```sh
# In build directory
ninja
```'''


def simulate_block():
    """Print simulate code block"""
    return '''
```sh
# In build directory
./simulate
```'''


def ninja_simulate_block():
    """Print simulate and ninja code block"""
    return '''
```sh
# In build directory
ninja && ./simulate
```'''


def cmake_check_script(state):
    return '''set(FINISH_COMPLETION_TEXT "%s")
set(START_COMPLETION_TEXT "%s")
configure_file(${SEL4_TUTORIALS_DIR}/tools/expect.py ${CMAKE_BINARY_DIR}/check @ONLY)
include(simulation)
GenerateSimulateScript()
''' % (state.print_completion(TaskContentType.COMPLETED), state.print_completion(TaskContentType.BEFORE))


def tutorial_init(name):
    return '''```sh
# For instructions about obtaining the tutorial sources see https://docs.sel4.systems/Tutorials/get-the-tutorials
#
# Follow these instructions to initialise the tutorial
# initialising the build directory with a tutorial exercise
./init --tut %s
# building the tutorial exercise
cd %s_build
ninja
```
''' % (name, name)


def tutorial_init_with_solution(name):
    return '''```sh
# For instructions about obtaining the tutorial sources see https://docs.sel4.systems/Tutorials/get-the-tutorials
#
# Follow these instructions to initialise the tutorial
# initialising the build directory with a tutorial exercise
./init --solution --tut %s
# building the tutorial exercise
cd %s_build
ninja
```
''' % (name, name)
