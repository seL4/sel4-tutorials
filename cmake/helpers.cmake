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

# We are including camkes from a non standard location and need to give some paths
# This is a very non standard way of using CAmkES as normally you would use the
# default top level CMakeLists.txt from CAmkES, but as we wish to switch CAmkES on and
# off this is not possible, hence we have to do this mangling here ourselves.
set(PYTHON_CAPDL_PATH "${CMAKE_SOURCE_DIR}/projects/camkes/capdl/python-capdl-tool")
set(CAPDL_TOOL_HELPERS "${CMAKE_SOURCE_DIR}/projects/camkes/capdl/capDL-tool/capDL-tool.cmake")
find_program(TPP_TOOL tpp PATHS "${CMAKE_SOURCE_DIR}/tools/camkes/tools")

macro(ImportCapDL)
    set(CapDLLoaderMaxObjects 20000 CACHE STRING "" FORCE)
    set(KernelRootCNodeSizeBits 17 CACHE STRING "" FORCE)
    set(KernelNumDomains 1 CACHE STRING "" FORCE)
    add_subdirectory("${CMAKE_SOURCE_DIR}/projects/camkes/capdl/capdl-loader-app" capdl-loader-app)
    include(${CAPDL_TOOL_HELPERS})
    CapDLToolInstall(install_capdl_tool CAPDL_TOOL_BINARY)
    include("${CAPDL_LOADER_BUILD_HELPERS}")

endmacro()

# Import camkes functions into caller scope
macro(ImportCamkes)
    set(CapDLLoaderMaxObjects 20000 CACHE STRING "" FORCE)
    set(KernelRootCNodeSizeBits 17 CACHE STRING "" FORCE)
    set(KernelNumDomains 1 CACHE STRING "" FORCE)
    include("${CMAKE_SOURCE_DIR}/tools/camkes/camkes.cmake")
    add_subdirectory("${CMAKE_SOURCE_DIR}/projects/camkes/capdl/capdl-loader-app" capdl-loader-app)
    add_subdirectory("${CMAKE_SOURCE_DIR}/tools/camkes/libsel4camkes" libsel4camkes)
endmacro()

macro(ImportCamkesVM)
    include("${CMAKE_SOURCE_DIR}/projects/camkes/vm/camkes_vm_settings.cmake")
    ImportCamkes()
    include("${CMAKE_SOURCE_DIR}/projects/camkes/global-components/global-components.cmake")
    add_subdirectory("${CMAKE_SOURCE_DIR}/projects/camkes/vm" vm)
    add_subdirectory("${CMAKE_SOURCE_DIR}/projects/camkes/vm-linux" vm-linux)
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

# Update all the unmodified files in target dir with source_dir if they haven't changed with respect to old_dir
# Report a conflict if a file in source_dir and target_dir are different with respect to old_dir
function(UpdateGeneratedFiles source_dir target_dir old_dir files)

    separate_arguments(file_list NATIVE_COMMAND ${files})
    foreach(file ${file_list})
        string(REPLACE ${source_dir} ${old_dir} old ${file})
        string(REPLACE ${source_dir} ${target_dir} target ${file})
        CopyIfUpdated(${file} ${target} ${old})
    endforeach()

endfunction()

# Run the command located in .tute_config in the input_dir.
# This command is expected to generate files in output_dir and report
# a list of dependent input files and generated output files.
# the reported input files will be added as a cmake dependency and will
# trigger the generation if they are modified.
function(ExecuteGenerationProcess input_dir output_dir generated_files)
    set(input_files ${CMAKE_CURRENT_BINARY_DIR}/input_files)
    set(output_files ${CMAKE_CURRENT_BINARY_DIR}/output_files)

    include(${CMAKE_SOURCE_DIR}/${input_dir}/.tute_config)

    execute_process(
        COMMAND ${CMAKE_COMMAND} -E env ${TUTE_COMMAND}
        OUTPUT_VARIABLE OUTPUT
        ERROR_VARIABLE OUTPUT
        RESULT_VARIABLE res
    )
    if (res)
        message(FATAL_ERROR "Failed to render: ${TUTE_COMMAND}, ${OUTPUT}")
    endif()
    # Set cmake to regenerate if any of the input files to the TUTE_COMMAND are updated
    file(READ "${input_files}" files)
    separate_arguments(file_list NATIVE_COMMAND ${files})
    set_property(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${file_list}")
    file(READ "${output_files}" files)
    set(${generated_files} ${files} PARENT_SCOPE)
endfunction()

# Generate a tutorial in dir.
# This will run a CMAKE_COMMAND in .tute_config from inside dir.
# It will copy the generated files into dir.
# If GenerateTutorial is rerun, any generated files that change
# will be updated in dir, unless there is a conflict.  A conflict
# is when the file in dir has been modified since the last generation.
function(GenerateTutorial dir)

    if (EXISTS ${CMAKE_SOURCE_DIR}/${dir}/.tute_config)
        set(output_dir ${CMAKE_CURRENT_BINARY_DIR}/${dir}/gen)
        set(old_output_dir ${CMAKE_CURRENT_BINARY_DIR}/${dir}/old)
        set(target_dir ${CMAKE_SOURCE_DIR}/${dir})

        ExecuteGenerationProcess(${dir} ${output_dir} generated_files)
        UpdateGeneratedFiles(${output_dir} ${target_dir} ${old_output_dir} ${generated_files})
    endif()
    if (NOT EXISTS ${CMAKE_SOURCE_DIR}/${dir}/CMakeLists.txt)
        message(FATAL_ERROR "Could not find: ${CMAKE_SOURCE_DIR}/${dir}/CMakeLists.txt"
                "It is required that ${CMAKE_SOURCE_DIR}/${dir} contains a CMakeLists.txt")
    endif()

endfunction()


file(GLOB_RECURSE capdl_python ${PYTHON_CAPDL_PATH}/*.py)


set(python_with_capdl ${CMAKE_COMMAND} -E env PYTHONPATH=${PYTHON_CAPDL_PATH} python)
set(capdl_linker_tool ${python_with_capdl} ${CMAKE_SOURCE_DIR}/projects/camkes/capdl/cdl_utils/capdl_linker.py)

function(DeclareCDLRootImage cdl cdl_target)
    cmake_parse_arguments(PARSE_ARGV 2 CDLROOTTASK "" "" "ELF;ELF_DEPENDS")
    if (NOT "${CDLROOTTASK_UNPARSED_ARGUMENTS}" STREQUAL "")
        message(FATAL_ERROR "Unknown arguments to DeclareCDLRootImage")
    endif()

    CapDLToolCFileGen(${cdl_target}_cspec ${cdl_target}_cspec.c ${cdl} "${CAPDL_TOOL_BINARY}"
        MAX_IRQS ${CapDLLoaderMaxIRQs}
        DEPENDS ${cdl_target} install_capdl_tool "${CAPDL_TOOL_BINARY}")

    # Ask the CapDL tool to generate an image with our given copied/mangled instances
    BuildCapDLApplication(
        C_SPEC "${cdl_target}_cspec.c"
        ELF ${CDLROOTTASK_ELF}
        DEPENDS ${CDLROOTTASK_ELF_DEPENDS} ${cdl_target}_cspec
        OUTPUT "capdl-loader"
    )
    DeclareRootserver("capdl-loader")
endfunction()



function(cdl_ld outfile output_target)
    cmake_parse_arguments(PARSE_ARGV 2 CDL_LD "" "" "ELF;MANIFESTS;DEPENDS")
    if (NOT "${CDL_LD_UNPARSED_ARGUMENTS}" STREQUAL "")
        message(FATAL_ERROR "Unknown arguments to cdl_ld")
    endif()

    add_custom_command(OUTPUT "${outfile}"
        COMMAND ${python_with_capdl} ${CDL_LD_MANIFESTS} |
        ${capdl_linker_tool}
            --arch=${KernelSel4Arch}
            gen_cdl
            --manifest-in -
            --elffile ${CDL_LD_ELF}
            --outfile ${outfile}
        DEPENDS ${CDL_LD_ELF} ${capdl_python} ${CDL_LD_MANIFESTS})
    add_custom_target(${output_target}
        DEPENDS "${outfile}")
    add_dependencies(${output_target} ${CDL_LD_DEPENDS})

endfunction()


function(cdl_pp manifest_in target)
    cmake_parse_arguments(PARSE_ARGV 2 CDL_PP "" "" "ELF;CFILE;DEPENDS")
    if (NOT "${CDL_PP_UNPARSED_ARGUMENTS}" STREQUAL "")
        message(FATAL_ERROR "Unknown arguments to cdl_pp")
    endif()

    add_custom_command(OUTPUT ${CDL_PP_CFILE}
        COMMAND ${python_with_capdl} ${manifest_in} |
        ${capdl_linker_tool}
                --arch=${KernelSel4Arch}
                build_cnode
                --manifest-in=-
                --elffile ${CDL_PP_ELF}
                --ccspace ${CDL_PP_CFILE}
        DEPENDS  ${capdl_python} ${manifest_in} )
    add_custom_target(${target} DEPENDS ${CDL_PP_CFILE})
endfunction()
