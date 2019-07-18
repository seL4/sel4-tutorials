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

cmake_minimum_required(VERSION 3.8.2)

add_library(sel4tutorials STATIC EXCLUDE_FROM_ALL src/constructors.c src/alloc.c)

target_link_libraries(
    sel4tutorials
    sel4_autoconf
    sel4runtime
    muslc
    sel4
    sel4platsupport
    sel4muslcsys
    sel4runtime_Config
)

# We force a dependency on the constructor symbol otherwise the linker won't link in the file
target_link_libraries(sel4tutorials -Wl,-u -Wl,register_debug_putchar)
target_include_directories(sel4tutorials PUBLIC include)
