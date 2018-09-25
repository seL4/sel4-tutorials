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

project(sel4_tutorials C)

add_subdirectory(libsel4tutorials)

# Only do the following if TUTORIAL_DIR is set, which indicates a tutorial
# to generate and run
if (NOT "${TUTORIAL_DIR}" STREQUAL "")
    # Save the TUTORIAL_DIR to the cache so that it persists across reruns
    set(TUTORIAL_DIR ${TUTORIAL_DIR} CACHE INTERNAL "")

    # Include cmake tutorial helper functions
    include(cmake/helpers.cmake)

    # generate tutorial sources into directory
    GenerateTutorial(${TUTORIAL_DIR})

    # Add the tutorial directory the same as any other project
    add_subdirectory(${CMAKE_SOURCE_DIR}/${TUTORIAL_DIR} ${CMAKE_BINARY_DIR}/${TUTORIAL_DIR})

    # generate a simulate script. This is here because we assume that all tutorials are simulatable
    GenerateSimulateScript()

endif()
