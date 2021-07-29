#
# Copyright 2021 Michael Neises
#
# SPDX-License-Identifier: BSD-2-Clause
#
cmake_minimum_required(VERSION 3.8.2)
if(NOT MODULE_HELPERS_FILE)
    message(FATAL_ERROR "MODULE_HELPERS_FILE is not defined")
endif()
include("${MODULE_HELPERS_FILE}")
DefineLinuxModule(${CMAKE_CURRENT_LIST_DIR}/hello hello-module hello-target KERNEL_DIR ${LINUX_KERNEL_DIR})
