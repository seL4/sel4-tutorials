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

set(project_dir "${CMAKE_CURRENT_LIST_DIR}")
get_filename_component(resolved_path ${CMAKE_CURRENT_LIST_FILE} REALPATH)
# repo_dir is distinct from project_dir as this file is symlinked.
# project_dir corresponds to the top level project directory, and
# repo_dir is the absolute path after following the symlink.
get_filename_component(repo_dir ${resolved_path} DIRECTORY)

include(${project_dir}/tools/seL4/cmake-tool/helpers/application_settings.cmake)

# Deal with the top level target-triplet variables.
if(NOT TUT_BOARD)
    message(
        FATAL_ERROR
            "Please select a board to compile for. Choose either pc or zynq7000\n\t`-DTUT_BOARD=<PREFERENCE>`"
    )
endif()

# Set arch and board specific kernel parameters here.
if(${TUT_BOARD} STREQUAL "pc")
    set(KernelArch "x86" CACHE STRING "" FORCE)
    set(KernelPlatform "pc99" CACHE STRING "" FORCE)
    if(${TUT_ARCH} STREQUAL "ia32")
        set(KernelSel4Arch "ia32" CACHE STRING "" FORCE)
    elseif(${TUT_ARCH} STREQUAL "x86_64")
        set(KernelSel4Arch "x86_64" CACHE STRING "" FORCE)
    else()
        message(FATAL_ERROR "Unsupported PC architecture ${TUT_ARCH}")
    endif()
elseif(${TUT_BOARD} STREQUAL "zynq7000")
    # Do a quick check and warn the user if they haven't set
    # -DARM/-DAARCH32/-DAARCH64.
    if(
        (NOT ARM)
        AND (NOT AARCH32)
        AND ((NOT CROSS_COMPILER_PREFIX) OR ("${CROSS_COMPILER_PREFIX}" STREQUAL ""))
    )
        message(
            WARNING
                "The target machine is an ARM machine. Unless you've defined -DCROSS_COMPILER_PREFIX, you may need to set one of:\n\t-DARM/-DAARCH32/-DAARCH64"
        )
    endif()

    set(KernelArch "arm" CACHE STRING "" FORCE)
    set(KernelSel4Arch "aarch32" CACHE STRING "" FORCE)
    set(KernelPlatform "zynq7000" CACHE STRING "" FORCE)
    ApplyData61ElfLoaderSettings(${KernelPlatform} ${KernelSel4Arch})
else()
    message(FATAL_ERROR "Unsupported board ${TUT_BOARD}.")
endif()

include(${project_dir}/kernel/configs/seL4Config.cmake)
set(CapDLLoaderMaxObjects 20000 CACHE STRING "" FORCE)
set(KernelRootCNodeSizeBits 16 CACHE STRING "")

# For the tutorials that do initialize the plat support serial printing they still
# just want to use the kernel debug putchar if it exists
set(LibSel4PlatSupportUseDebugPutChar true CACHE BOOL "" FORCE)

# Just let the regular abort spin without calling DebugHalt to prevent needless
# confusing output from the kernel for a tutorial
set(LibSel4MuslcSysDebugHalt FALSE CACHE BOOL "" FORCE)

# Only configure a single domain for the domain scheduler
set(KernelNumDomains 1 CACHE STRING "" FORCE)

# We must build the debug kernel because the tutorials rely on seL4_DebugPutChar
# and they don't initialize a platsupport driver.
ApplyCommonReleaseVerificationSettings(FALSE FALSE)

# We will attempt to generate a simulation script, so try and generate a simulation
# compatible configuration
ApplyCommonSimulationSettings(${KernelArch})
if(FORCE_IOMMU)
    set(KernelIOMMU ON CACHE BOOL "" FORCE)
endif()

if(NOT "${TUTORIAL_DIR}" STREQUAL "")
    include("${TUTORIAL_DIR}/settings.cmake" OPTIONAL)
endif()
