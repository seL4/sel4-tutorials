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

project(dynamic-3 C)

add_executable(app app.c)
target_link_libraries(app sel4 muslc utils sel4tutorials
                      sel4muslcsys sel4platsupport sel4utils sel4debug)
set_property(TARGET app APPEND_STRING PROPERTY LINK_FLAGS " -u __vsyscall_ptr ")

MakeCPIO(archive.o "$<TARGET_FILE:app>")

add_executable(dynamic-3 archive.o main.c)

target_link_libraries(dynamic-3
    sel4
    muslc utils sel4tutorials
    sel4muslcsys sel4platsupport sel4utils sel4debug sel4allocman)

DeclareRootserver(dynamic-3)
/*? macros.cmake_check_script(state) ?*/
