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

# Import camkes functions into caller scope
macro(ImportCamkes)
    set(CapDLLoaderMaxObjects 20000 CACHE STRING "" FORCE)
    set(KernelRootCNodeSizeBits 17 CACHE STRING "" FORCE)
    set(KernelNumDomains 1 CACHE STRING "" FORCE)

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
endmacro()

# Helper that takes a filename and makes the directory where that file would go if
function(EnsureDir filename)
    get_filename_component(dir "${filename}" DIRECTORY)
    file(MAKE_DIRECTORY "${dir}")
endfunction(EnsureDir)

# Wrapper around `file(RENAME` that ensures the rename succeeds by creating the destination
# directory if it does not exist
function(Rename src dest)
    EnsureDir("${dest}")
    file(RENAME "${src}" "${dest}")
endfunction(Rename)

# Wrapper around using `cmake -E copy` that tries to ensure the copy succeeds by first
# creating the destination directory if it does not exist
function(Copy src dest)
    EnsureDir("${dest}")
    execute_process(
        COMMAND ${CMAKE_COMMAND} -E copy "${src}" "${dest}"
        RESULT_VARIABLE exit_status
    )
    if (NOT ("${exit_status}" EQUAL 0))
        message(FATAL_ERROR "Failed to copy ${src} to ${dest}")
    endif()
endfunction(Copy)

# Return non-zero if files one and two are not the same.
function(DiffFiles res one two)
        execute_process(
            COMMAND diff -q "${one}" "${two}"
            RESULT_VARIABLE exit_status
            OUTPUT_VARIABLE OUTPUT
            ERROR_VARIABLE OUTPUT
        )
        set(${res} ${exit_status} PARENT_SCOPE)
endfunction()

# Try to update output and old with the value of input
# Only update output with input if input != old
# Fail if input != old and output != old
function(CopyIfUpdated input output old)
    if (EXISTS ${output})
        DiffFiles(template_updated ${input} ${old})
        DiffFiles(instance_updated ${output} ${old})
        if(template_updated AND instance_updated)
            message(FATAL_ERROR "Template has been updated and the instantiated tutorial has been updated. \
             Changes would be lost if proceeded.")
        endif()
        set(do_update ${template_updated})
    else()
        set(do_update TRUE)
    endif()
    if(do_update)
        Copy(${input} ${output})
        Copy(${input} ${old})
    endif()
    file(REMOVE ${input})

endfunction()
