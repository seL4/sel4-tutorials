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
ImportCapDL()
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -u __vsyscall_ptr")

/*? write_manifest(".manifest.obj") ?*/
cdl_pp(${CMAKE_CURRENT_SOURCE_DIR}/.manifest.obj cdl_pp_target
	/*- for (elf, file) in state.stash.elfs.iteritems() -*/
    ELF "/*?elf?*/"
    CFILE "${CMAKE_CURRENT_BINARY_DIR}/cspace_/*?elf?*/.c"
    /*- endfor -*/
)

/*- for (elf, file) in state.stash.elfs.iteritems() -*/
add_executable(/*?elf?*/ EXCLUDE_FROM_ALL /*?file['filename']?*/ cspace_/*?elf?*/.c)
add_dependencies(/*?elf?*/ cdl_pp_target)
target_link_libraries(/*?elf?*/ sel4tutorials)

list(APPEND elf_files "$<TARGET_FILE:/*?elf?*/>")
list(APPEND elf_targets "/*?elf?*/")

/*- endfor -*/


cdl_ld("${CMAKE_CURRENT_BINARY_DIR}/spec.cdl" capdl_spec
    MANIFESTS ${CMAKE_CURRENT_SOURCE_DIR}/.manifest.obj
    ELF ${elf_files}
    DEPENDS ${elf_targets})

DeclareCDLRootImage("${CMAKE_CURRENT_BINARY_DIR}/spec.cdl" capdl_spec ELF ${elf_files} ELF_DEPENDS ${elf_targets})


/*? macros.cmake_check_script(state) ?*/
