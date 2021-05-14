#
# Copyright 2018, Data61, CSIRO (ABN 41 687 119 230)
#
# SPDX-License-Identifier: BSD-2-Clause
#

include(${SEL4_TUTORIALS_DIR}/settings.cmake)
sel4_tutorials_regenerate_tutorial(${CMAKE_CURRENT_SOURCE_DIR})

cmake_minimum_required(VERSION 3.7.2)
# declare the dynamic-4 CMake project and the languages it is written in
project(dynamic-4 C ASM)

sel4_tutorials_setup_roottask_tutorial_environment()

add_executable(client client.c)

target_link_libraries(client
    sel4
    muslc utils sel4tutorials
    sel4muslcsys sel4platsupport sel4utils sel4debug)


include(cpio)
MakeCPIO(archive.o "$<TARGET_FILE:client>")

add_executable(dynamic-4 archive.o main.c)

target_link_libraries(dynamic-4
    sel4runtime sel4
    muslc utils sel4tutorials
    sel4muslcsys sel4platsupport sel4utils sel4debug sel4allocman)

include(rootserver)
DeclareRootserver(dynamic-4)
/*? macros.cmake_check_script(state) ?*/
