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

if (CAMKES)
    # We are including camkes from a non standard location and need to give some paths
    # This is a very non standard way of using CAmkES as normally you would use the
    # default top level CMakeLists.txt from CAmkES, but as we wish to switch CAmkES on and
    # off this is not possible, hence we have to do this mangling here ourselves.
    set(PYTHON_CAPDL_PATH "${CMAKE_SOURCE_DIR}/projects/camkes/capdl/python-capdl-tool")
    set(CAPDL_TOOL_HELPERS "${CMAKE_SOURCE_DIR}/projects/camkes/capdl/capDL-tool/capDL-tool.cmake")
    find_program(TPP_TOOL tpp PATHS "${CMAKE_SOURCE_DIR}/tools/camkes/tools")
    include("${CMAKE_SOURCE_DIR}/tools/camkes/camkes.cmake")
    add_subdirectory("${CMAKE_SOURCE_DIR}/projects/camkes/capdl/capdl-loader-app" capdl-loader-app)
    include("${CMAKE_SOURCE_DIR}/projects/camkes/global-components/global-components.cmake")
    add_subdirectory("${CMAKE_SOURCE_DIR}/tools/camkes/libsel4camkes" libsel4camkes)
endif()

add_subdirectory(libsel4tutorials)
add_subdirectory(tutorials/${TUTORIAL})

if (CAMKES)
    GenerateCAmkESRootserver()
else()
    if (NOT (TARGET ${TUTORIAL}))
        message(FATAL_ERROR "bug: ${TUTORIAL} did not define a target with the same name")
    endif()

    DeclareRootserver(${TUTORIAL})
endif()

GenerateSimulateScript()

# Print out any information regarding to sources that were constructed
# whilst processing the tutorial. This is guarded by a config option so that
# it can be toggled externally to prevent the instructions printing out every
# time cmake runs.
if (TUTORIALS_PRINT_INSTRUCTIONS)
    get_property(template_files GLOBAL PROPERTY template_files)
    get_property(companion_files GLOBAL PROPERTY companion_files)
    if (NOT ("${companion_files}" STREQUAL ""))
        message("The following files do not require modifications but are copied to provide context:")
        foreach(file IN LISTS companion_files)
            message("\t${file}")
        endforeach()
    endif()
    if (NOT ("${template_files}" STREQUAL ""))
        if (BUILD_SOLUTIONS)
            message("Tutorial solutions placed in:")
        else()
            message("The following files need to be modified to complete the tutorial")
        endif()
        foreach(file IN LISTS template_files)
            message("\t${file}")
        endforeach()
    endif()
endif()
