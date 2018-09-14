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

from tutorialstate import TaskContentType
def ninja_block():
	'''
	Print ninja code block
	'''
	return '''
```sh
# In build directory
ninja
```'''


def simulate_block():
	'''
	Print simulate code block
	'''
	return '''
```sh
# In build directory
./simulate
```'''


def ninja_simulate_block():
	'''
	Print simulate and ninja code block
	'''
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
* [Debugging guide](https://docs.sel4.systems/DebuggingGuide.html)
* [Developer's mailing list](https://sel4.systems/lists/listinfo/devel)
'''

def cmake_check_script(state):
	return '''set(FINISH_COMPLETION_TEXT "%s")
set(START_COMPLETION_TEXT "%s")
configure_file(${CMAKE_SOURCE_DIR}/projects/sel4-tutorials/tools/expect.py ${CMAKE_BINARY_DIR}/check @ONLY)
''' % (state.print_completion(TaskContentType.COMPLETED), state.print_completion(TaskContentType.BEFORE))
