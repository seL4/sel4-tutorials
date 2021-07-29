#
# Copyright 2021 Michael Neises
#
# SPDX-License-Identifier: BSD-2-Clause
#

cmake_minimum_required(VERSION 3.8.2)

project(module-minimal C)

include(${CAMKES_ARM_VM_HELPERS_PATH})

# Create our CPP Flags based on ARM VM config variables
find_package(camkes-vm-linux REQUIRED)
include(${CAMKES_VM_LINUX_HELPERS_PATH})
set(cpp_flags "-DKERNELARMPLATFORM_QEMU-ARM-VIRT")

# Add our custom linux kernel
AddToFileServer("linux" "${CAMKES_VM_IMAGES_DIR}/qemu-arm-virt/newLinux")

# Grab the default rootfs
set(rootfs_file "${CAMKES_VM_IMAGES_DIR}/qemu-arm-virt/rootfs.cpio.gz")

include(${CAMKES_VM_LINUX_SOURCE_HELPERS_PATH})
include(${CAMKES_VM_LINUX_MODULE_HELPERS_PATH})
include(ExternalProject)
include(external-project-helpers)

# Setup and configure linux sources
set(linux_config "${CAMKES_VM_LINUX_DIR}/linux_configs/4.9.y/64/config")
set(linux_symvers "${CAMKES_VM_LINUX_DIR}/linux_configs/4.9.y/64/Module.symvers")

# Set the directory of our custom linux kernel
set(linux_dir "${CAMKES_VM_IMAGES_DIR}/qemu-arm-virt/linux-stable")

# Add the external hello module project
ExternalProject_Add(hello-module
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/modules
    BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/hello-module
    BUILD_ALWAYS ON
    STAMP_DIR ${CMAKE_CURRENT_BINARY_DIR}/hello-module-stamp
    EXCLUDE_FROM_ALL
    INSTALL_COMMAND ""
    CMAKE_ARGS
        -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
        -DCMAKE_TOOLCHAIN_FILE=${LINUX_64BIT_TOOLCHAIN}
        -DLINUX_KERNEL_DIR=${linux_dir}
        -DMODULE_HELPERS_FILE=${CAMKES_VM_LINUX_DIR}/linux-module-helpers.cmake
)

# Add our module binary to the overlay
# The number at this time happens to be 275.
AddExternalProjFilesToOverlay(
    hello-module
    ${CMAKE_CURRENT_BINARY_DIR}/hello-module
    vm-overlay
    "lib/modules/4.9.275/kernel/drivers/vmm"
    FILES
    hello.ko
)

# Add our module's init script- just an insmod call
AddFileToOverlayDir(
    "init"
    ${CMAKE_CURRENT_SOURCE_DIR}/init
    "/etc/init.d"
    vm-overlay
)

# Generate overlayed rootfs
AddOverlayDirToRootfs(
    vm-overlay
    ${rootfs_file}
    "buildroot"
    "rootfs_install"
    output_overlayed_rootfs_location
    rootfs_target
    GZIP
)

# Add the newly built rootfs
AddToFileServer(
    "linux-initrd"
    ${output_overlayed_rootfs_location}
    DEPENDS
    rootfs_target
)

# Updated dtb based on initrd
UpdateDtbFromInitrd(
    "${CAMKES_VM_IMAGES_DIR}/qemu-arm-virt/linux-dtb"
    ${rootfs_file}
    "0x4d700000"
    dtb_gen_target
    output_dtb_location
)
AddToFileServer("linux-dtb" "${output_dtb_location}" DEPENDS dtb_gen_target)
include(simulation)
set(SIMULATION ON CACHE BOOL "Generate simulation script to run qemu with the proper arguments")
if(SIMULATION)
    GenerateSimulateScript()
endif()

AddCamkesCPPFlag(cpp_flags CONFIG_VARS VmEmmc2NoDMA VmVUSB Tk1DeviceFwd Tk1Insecure)

DefineCAmkESVMFileServer()

CAmkESAddImportPath(${KernelARMPlatform})

# Declare root server
DeclareCAmkESRootserver(
    module_minimal.camkes
    CPP_FLAGS
    ${cpp_flags}
    CPP_INCLUDES
    ${CAMKES_VM_DIR}/components/VM_Arm
)