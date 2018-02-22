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

if (${tut_desired_tutorial} STREQUAL "hello-4-app")
    message(FATAL_ERROR "\tPlease build the hello-4 tutorial instead. This sub-app is not meant to be targeted directly.\n\tTry -DTUTORIAL=hello-4")
endif()

file(GLOB c_sources src/*.c src/*/*.c)
list(SORT c_sources)

add_executable(hello-4-app ${c_sources})

set_property(TARGET hello-4-app APPEND_STRING PROPERTY LINK_FLAGS " -u __vsyscall_ptr ")

target_link_libraries(hello-4-app
    sel4
    muslc utils
    sel4muslcsys sel4platsupport sel4utils sel4debug)

target_include_directories(hello-4-app PRIVATE "include")
