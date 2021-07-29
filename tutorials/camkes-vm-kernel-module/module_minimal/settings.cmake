#
# Copyright 2021 Michael Neises
#
# SPDX-License-Identifier: BSD-2-Clause
#

set(supported "qemu-arm-virt")
if(NOT "${PLATFORM}" IN_LIST supported)
    message(FATAL_ERROR "PLATFORM: ${PLATFORM} not supported.
         Supported: ${supported}")
endif()

set(LibUSB OFF CACHE BOOL "" FORCE)

# force cpu
set(QEMU_MEMORY "2048")
set(KernelArmCPU cortex-a53 CACHE STRING "" FORCE)
set(VmInitRdFile ON CACHE BOOL "" FORCE)
