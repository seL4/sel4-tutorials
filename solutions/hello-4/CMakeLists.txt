#
# Copyright 2018, Data61
# Commonwealth Scientific and Industrial Research Organisation (CSIRO)
# ABN 41 687 119 230.
#
# This software may be distributed and modified according to the terms of
# the BSD 2-Clause license. Note that NO WARRANTY is provided.
# See "LICENSE_BSD2.txt" for details.
#
# @TAG(DATA61_BSD)
#

cmake_minimum_required(VERSION 3.7.2)

add_subdirectory(../hello-4-app hello-4-app)

file(GLOB c_sources src/*.c src/*/*.c)
list(SORT c_sources)

MakeCPIO(sel4_tutorial_cpio.o "$<TARGET_FILE:hello-4-app>")

add_executable(sel4_tutorial_executable ${c_sources} sel4_tutorial_cpio.o)

target_link_libraries(sel4_tutorial_executable
    sel4
    muslc utils
    sel4muslcsys sel4platsupport sel4utils sel4debug sel4allocman)

target_include_directories(sel4_tutorial_executable PRIVATE "include")
