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

project(hello-timer C)

add_subdirectory(hello-timer-client)

MakeCPIO(archive.o "$<TARGET_FILE:hello-timer-client>")

add_tutorials_executable(hello-timer archive.o TEMPLATE_SOURCES src/main.c TUTORIAL_SOURCES src/util.c)

target_link_libraries(hello-timer
    sel4
    muslc utils
    sel4muslcsys sel4platsupport sel4utils sel4debug sel4allocman)
