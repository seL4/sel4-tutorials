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

# See if a valid tutorial was defined
if (NOT (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tutorials/${TUTORIAL}/CMakeLists.txt"))
    message(FATAL_ERROR "\"${TUTORIAL}\" is not a valid tutorial. Choose one with -DTUTORIAL=<PREFERENCE>")
endif()

if ("${TUTORIAL}" MATCHES "camkes")
    set(CAMKES TRUE)
else ()
    set(CAMKES FALSE)
endif()

include(cmake/settings.cmake)

# Ensure we have the python script for processing the templates
RequireFile(TemplateTool "template.py")
# Force cmake to rerun if the template tool changes
set_property(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${TemplateTool}")

# Save the tutorial directory as this is where we want to define all our tutorial paths to
# be relative to.
set(tutorial_base_dir "${CMAKE_CURRENT_SOURCE_DIR}/tutorials")

include(cmake/helpers.cmake)


add_subdirectory(libsel4tutorials)

# Save the TUTORIAL_DIR to the cache so that it persists across reruns
set(TUTORIAL_DIR ${TUTORIAL_DIR} CACHE INTERNAL "")

# generate tutorial sources into directory
GenerateTutorial(${TUTORIAL_DIR})

# Add the tutorial directory the same as any other project
add_subdirectory(${CMAKE_SOURCE_DIR}/${TUTORIAL_DIR} ${CMAKE_BINARY_DIR}/${TUTORIAL_DIR})

GenerateSimulateScript()
