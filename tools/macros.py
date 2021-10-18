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


def help_block():
    return '''
---
## Getting help
Stuck? See the resources below.
* [FAQ](https://docs.sel4.systems/FrequentlyAskedQuestions)
* [seL4 Manual](http://sel4.systems/Info/Docs/seL4-manual-latest.pdf)
* [Debugging guide](https://docs.sel4.systems/DebuggingGuide.html)
* [seL4 Discourse forum](https://sel4.discourse.group)
* [Developer's mailing list](https://lists.sel4.systems/postorius/lists/devel.sel4.systems/)
* [Mattermost Channel](https://mattermost.trustworthy.systems/sel4-external/)
'''


def cmake_check_script(state):
    return '''set(FINISH_COMPLETION_TEXT "%s")
set(START_COMPLETION_TEXT "%s")
configure_file(${SEL4_TUTORIALS_DIR}/tools/expect.py ${CMAKE_BINARY_DIR}/check @ONLY)
include(simulation)
GenerateSimulateScript()
''' % (state.print_completion(TaskContentType.COMPLETED), state.print_completion(TaskContentType.BEFORE))


def tutorial_init(name):
    return '''```sh
# For instructions about obtaining the tutorial sources see https://docs.sel4.systems/Tutorials/#get-the-code
#
# Follow these instructions to initialise the tutorial
# initialising the build directory with a tutorial exercise
./init --tut %s
# building the tutorial exercise
cd %s_build
ninja
```
''' % (name, name)
