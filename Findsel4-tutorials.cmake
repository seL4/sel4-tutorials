#
# Copyright 2019, Data61, CSIRO (ABN 41 687 119 230)
#
# SPDX-License-Identifier: BSD-2-Clause
#

set(SEL4_TUTORIALS_DIR "${CMAKE_CURRENT_LIST_DIR}" CACHE STRING "")
mark_as_advanced(SEL4_TUTORIALS_DIR)

# Include cmake tutorial helper functions
include(${SEL4_TUTORIALS_DIR}/cmake/helpers.cmake)

macro(sel4_tutorials_regenerate_tutorial tutorial_dir)
    # generate tutorial sources into directory

    GenerateTutorial(${tutorial_dir})
    if("${CMAKE_CURRENT_LIST_FILE}" STREQUAL "${CMAKE_CURRENT_SOURCE_DIR}/CMakeLists.txt")
        get_property(tute_hack GLOBAL PROPERTY DONE_TUTE_HACK)
        if(NOT tute_hack)
            set_property(GLOBAL PROPERTY DONE_TUTE_HACK TRUE)
            # We are in the main project phase and regenerating the tutorial
            # may have updated the file calling us.  So we do some magic...
            include(${CMAKE_CURRENT_SOURCE_DIR}/CMakeLists.txt)
            return()
        endif()
    endif()

endmacro()

macro(sel4_tutorials_import_libsel4tutorials)
    add_subdirectory(${SEL4_TUTORIALS_DIR}/libsel4tutorials libsel4tutorials)
endmacro()

macro(sel4_tutorials_setup_roottask_tutorial_environment)

    find_package(seL4 REQUIRED)
    find_package(elfloader-tool REQUIRED)
    find_package(musllibc REQUIRED)
    find_package(util_libs REQUIRED)
    find_package(seL4_libs REQUIRED)

    sel4_import_kernel()
    elfloader_import_project()

    # This sets up environment build flags and imports musllibc and runtime libraries.
    musllibc_setup_build_environment_with_sel4runtime()
    sel4_import_libsel4()
    util_libs_import_libraries()
    sel4_libs_import_libraries()
    sel4_tutorials_import_libsel4tutorials()

endmacro()

macro(sel4_tutorials_setup_capdl_tutorial_environment)
    sel4_tutorials_setup_roottask_tutorial_environment()
    capdl_import_project()
    CapDLToolInstall(install_capdl_tool CAPDL_TOOL_BINARY)
endmacro()

include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(camkes-tool DEFAULT_MSG SEL4_TUTORIALS_DIR)
