
#
# Copyright 2018, Data61, CSIRO (ABN 41 687 119 230)
#
# SPDX-License-Identifier: BSD-2-Clause
#
include(${SEL4_TUTORIALS_DIR}/settings.cmake)
sel4_tutorials_regenerate_tutorial(${CMAKE_CURRENT_SOURCE_DIR})

cmake_minimum_required(VERSION 3.7.2)
# declare the fault-handlers CMake project and the languages it is written in
project(fault-handlers C ASM)

sel4_tutorials_setup_capdl_tutorial_environment()

/*? write_manifest(manifest=".manifest.obj", allocator=".allocator.obj") ?*/
cdl_pp(${CMAKE_CURRENT_SOURCE_DIR}/.manifest.obj cdl_pp_target
	/*- for (elf, file) in state.stash.elfs.items() -*/
    ELF "/*?elf?*/"
    CFILE "${CMAKE_CURRENT_BINARY_DIR}/cspace_/*?elf?*/.c"
    /*- endfor -*/
)   

/*- for (elf, file) in state.stash.elfs.items() -*/
add_executable(/*?elf?*/ EXCLUDE_FROM_ALL /*?file['filename']?*/ cspace_/*?elf?*/.c)
add_dependencies(/*?elf?*/ cdl_pp_target)
target_link_libraries(/*?elf?*/ sel4tutorials)

list(APPEND elf_files "$<TARGET_FILE:/*?elf?*/>")
list(APPEND elf_targets "/*?elf?*/")

/*- endfor -*/


cdl_ld("${CMAKE_CURRENT_BINARY_DIR}/spec.cdl" capdl_spec 
    MANIFESTS ${CMAKE_CURRENT_SOURCE_DIR}/.allocator.obj
    ELF ${elf_files}
    KEYS ${elf_targets}
    DEPENDS ${elf_targets})

DeclareCDLRootImage("${CMAKE_CURRENT_BINARY_DIR}/spec.cdl" capdl_spec ELF ${elf_files} ELF_DEPENDS ${elf_targets})


/*? macros.cmake_check_script(state) ?*/
