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

project(sel4_tutorials C)

# Build a list of all valid tutorial names (e.g, hello-1, hello-2, etc) so we
# can validate the user's selected tutorial below.
file(GLOB tutorial_subdirs RELATIVE "${CMAKE_SOURCE_DIR}" "${CMAKE_SOURCE_DIR}/apps/*")
foreach(currsubdir ${tutorial_subdirs})
    if (IS_DIRECTORY "${CMAKE_SOURCE_DIR}/${currsubdir}")
        get_filename_component(currsubdir ${currsubdir} NAME)
        # Lowercase all of the tutorial names for comparison later.
        string(TOLOWER ${currsubdir} currsubdir)
        list(APPEND valid_tutorials_list "${currsubdir}")
    endif()
endforeach()

# This section seeks to set "tut_desired_tutorial" to indicate which tutorial
# subfolder should be built. A tutorial must be selected and is one is not
# selected, the build is terminated.
#
if (NOT TUTORIAL)
    message(FATAL_ERROR "You must select a tutorial for compilation. \
Select using\n\t-DTUTORIAL=<PREFERENCE>.\n\tChoose one of ${valid_tutorials_list}.")
else()
    # Lowercase for comparison.
    string(TOLOWER ${TUTORIAL} TUTORIAL)
    # Validate the tutorial name.
    if ("${TUTORIAL}" IN_LIST valid_tutorials_list)
        set(tut_desired_tutorial ${TUTORIAL})
    else()
        message(FATAL_ERROR "Invalid tutorial name '${TUTORIAL}'\n\tPlease choose one of ${valid_tutorials_list}.\n\t-DTUTORIAL=<PREFERENCE>")
    endif()
endif()

# This section selects either the exercises or the solutions for compilation.
# The resultant decision is stored in "tut_desired_sources".
#
if (TUT_DEV)
    # In the sel4tutorial-source repo we can choose on the fly.
    set(tut_desired_sources "apps-sel4-exercises")
    if (BUILD_SOLUTIONS)
        set(tut_desired_sources "apps-sel4-solutions")
    endif()
else()
    # In the generated repository the selection is made in the repo init command.
    set(tut_desired_sources "apps")
endif()

# Deal with the top level target-triplet variables.
if (NOT TUT_BOARD)
    message(FATAL_ERROR "Please select a board to compile for. Choose either pc or zynq7000\n\t`-DTUT_BOARD=<PREFERENCE>`")
endif()
if (NOT TUT_ARCH)
    set(TUT_ARCH "")
endif()

# Set arch and board specific kernel parameters here.
if (${TUT_BOARD} STREQUAL "pc")
    set(KernelArch "x86" CACHE STRING "" FORCE)
    if (${TUT_ARCH} STREQUAL "x86")
        set(KernelX86Sel4Arch "ia32" CACHE STRING "" FORCE)
    elseif (${TUT_ARCH} STREQUAL "x86_64")
        set(KernelX86Sel4Arch "x86_64" CACHE STRING "" FORCE)
    else()
        message(FATAL_ERROR "Unsupported PC architecture ${TUT_ARCH}")
    endif()
elseif(${TUT_BOARD} STREQUAL "zynq7000")
    # Do a quick check and warn the user if they haven't set
    # -DARM/-DAARCH32/-DAARCH64.
    if ((NOT ARM) AND (NOT AARCH32)
        AND ((NOT CROSS_COMPILER_PREFIX) OR ("${CROSS_COMPILER_PREFIX}" STREQUAL "")))
        message(WARNING "The target machine is an ARM machine. Unless you've defined -DCROSS_COMPILER_PREFIX, you may need to set one of:\n\t-DARM/-DAARCH32/-DAARCH64")
    endif()

    set(KernelArch "arm" CACHE STRING "" FORCE)
    set(KernelArmSel4Arch "aarch32" CACHE STRING "" FORCE)
    set(KernelARMPlatform "zynq7000" CACHE STRING "" FORCE)
else()
    message(FATAL_ERROR "Unsupported board ${TUT_BOARD}.")
endif()

##
# SECTION: Kernel configuration: configure the kernel for compilation.
#

# We must build the debug kernel because the tutorials rely on seL4_DebugPutChar
# and they don't initialize a platsupport driver.
set(KernelVerificationBuild OFF CACHE BOOL "" FORCE)
set(KernelDebugBuild true CACHE BOOL "" FORCE)
set(KernelPrinting true CACHE BOOL "" FORCE)

##
# SECTION: util_libs configuration
#

##
# SECTION: seL4_libs configuration
#

# Some of the tutorials redefine __arch_putchar to call seL4_DebugPutChar().
set(LibSel4MuslcSysArchPutcharWeak true CACHE BOOL "" FORCE)
# Yet others just rely on libplatsupport internally redirecting __serial_setup
# to call seL4_DebugPutChar.
set(LibSel4PlatSupportUseDebugPutChar true CACHE BOOL "" FORCE)

# Finally we build the set of CMakeLists.txt files that will be built and
# proceed.
#
# We must explicitly specify a binary output location since we're trying to
# build a source folder which is up at the top-level of the tree, and not
# relative to our current project's source folder.
add_subdirectory(
    ${CMAKE_SOURCE_DIR}/${tut_desired_sources}/${tut_desired_tutorial}
    ${CMAKE_BINARY_DIR}/${tut_desired_sources}/${tut_desired_tutorial})

DeclareRootserver(sel4_tutorial_executable)
