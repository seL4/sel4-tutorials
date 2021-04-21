#
# Copyright 2018, Data61, CSIRO (ABN 41 687 119 230)
#
# SPDX-License-Identifier: BSD-2-Clause
#

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
        COMMAND
            ${CMAKE_COMMAND} -E copy "${src}" "${dest}"
        RESULT_VARIABLE exit_status
    )
    if(NOT ("${exit_status}" EQUAL 0))
        message(FATAL_ERROR "Failed to copy ${src} to ${dest}")
    endif()
endfunction(Copy)

# Return non-zero if files one and two are not the same.
function(DiffFiles res one two)
    execute_process(
        COMMAND
            diff -q "${one}" "${two}"
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
    if(EXISTS ${output})
        DiffFiles(template_updated ${input} ${old})
        DiffFiles(instance_updated ${output} ${old})
        if(template_updated AND instance_updated)
            message(
                FATAL_ERROR
                    "Template has been updated and the instantiated tutorial has been updated. \
             Changes would be lost if proceeded."
            )
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
        string(
            REPLACE
                ${source_dir}
                ${old_dir}
                old
                ${file}
        )
        string(
            REPLACE
                ${source_dir}
                ${target_dir}
                target
                ${file}
        )
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

    include(${input_dir}/.tute_config)

    execute_process(
        COMMAND
            ${CMAKE_COMMAND} -E env ${TUTE_COMMAND}
        OUTPUT_VARIABLE OUTPUT
        ERROR_VARIABLE OUTPUT
        RESULT_VARIABLE res
    )
    if(res)
        message(FATAL_ERROR "Failed to render: ${TUTE_COMMAND}, ${OUTPUT}")
    endif()
    # Set cmake to regenerate if any of the input files to the TUTE_COMMAND are updated
    file(READ "${input_files}" files)
    separate_arguments(file_list NATIVE_COMMAND ${files})
    if("${CMAKE_CURRENT_LIST_FILE}" STREQUAL "${CMAKE_CURRENT_SOURCE_DIR}/CMakeLists.txt")
        set_property(
            DIRECTORY "${CMAKE_SOURCE_DIR}"
            APPEND
            PROPERTY CMAKE_CONFIGURE_DEPENDS "${file_list}"
        )
    endif()
    file(READ "${output_files}" files)
    set(${generated_files} ${files} PARENT_SCOPE)
endfunction()

# Generate a tutorial in dir.
# This will run a CMAKE_COMMAND in .tute_config from inside dir.
# It will copy the generated files into dir.
# If GenerateTutorial is rerun, any generated files that change
# will be updated in dir, unless there is a conflict.  A conflict
# is when the file in dir has been modified since the last generation.
function(GenerateTutorial full_path)
    get_filename_component(dir ${full_path} NAME)
    get_filename_component(base_dir ${full_path} DIRECTORY)

    # Implement include_guard() functionality within a function that
    # can only be called once. Using include_guard() is considered dangerous
    # within functions as it will be affected by other include_guard() calls
    # within functions within the same file.
    get_property(gen_tutorial GLOBAL PROPERTY GenerateTutorialONCE)
    if(NOT gen_tutorial)
        set_property(GLOBAL PROPERTY GenerateTutorialONCE TRUE)
    else()
        # If already generated the tutorial in this run we don't need to do so again.
        return()
    endif()

    if(EXISTS ${base_dir}/${dir}/.tute_config)
        set(output_dir ${CMAKE_CURRENT_BINARY_DIR}/.tutegen/${dir}/gen)
        set(old_output_dir ${CMAKE_CURRENT_BINARY_DIR}/.tutegen/${dir}/old)
        set(target_dir ${base_dir}/${dir})

        ExecuteGenerationProcess(${base_dir}/${dir} ${output_dir} generated_files)
        UpdateGeneratedFiles(${output_dir} ${target_dir} ${old_output_dir} ${generated_files})
    endif()
    if(NOT EXISTS ${base_dir}/${dir}/CMakeLists.txt)
        message(
            FATAL_ERROR
                "Could not find: ${base_dir}/${dir}/CMakeLists.txt"
                "It is required that ${base_dir}/${dir} contains a CMakeLists.txt"
        )
    endif()

endfunction()

find_package(capdl REQUIRED)
file(GLOB_RECURSE capdl_python ${PYTHON_CAPDL_PATH}/*.py)
set(PYTHON3 "python3" CACHE INTERNAL "")

set(python_with_capdl ${CMAKE_COMMAND} -E env PYTHONPATH=${PYTHON_CAPDL_PATH} ${PYTHON3})
set(capdl_linker_tool ${python_with_capdl} ${CAPDL_LINKER_TOOL})

function(DeclareCDLRootImage cdl cdl_target)
    cmake_parse_arguments(PARSE_ARGV 2 CDLROOTTASK "" "" "ELF;ELF_DEPENDS")
    if(NOT "${CDLROOTTASK_UNPARSED_ARGUMENTS}" STREQUAL "")
        message(FATAL_ERROR "Unknown arguments to DeclareCDLRootImage")
    endif()

    CapDLToolCFileGen(
        ${cdl_target}_cspec
        ${cdl_target}_cspec.c
        FALSE
        "$<TARGET_PROPERTY:object_sizes,FILE_PATH>"
        ${cdl}
        "${CAPDL_TOOL_BINARY}"
        MAX_IRQS
        ${CapDLLoaderMaxIRQs}
        DEPENDS
        ${cdl_target}
        install_capdl_tool
        "${CAPDL_TOOL_BINARY}"
    )

    # Ask the CapDL tool to generate an image with our given copied/mangled instances
    BuildCapDLApplication(
        C_SPEC
        "${cdl_target}_cspec.c"
        ELF
        ${CDLROOTTASK_ELF}
        DEPENDS
        ${CDLROOTTASK_ELF_DEPENDS}
        ${cdl_target}_cspec
        OUTPUT
        "capdl-loader"
    )
    include(rootserver)
    DeclareRootserver("capdl-loader")
endfunction()

function(cdl_ld outfile output_target)
    cmake_parse_arguments(PARSE_ARGV 2 CDL_LD "" "" "ELF;KEYS;MANIFESTS;DEPENDS")
    if(NOT "${CDL_LD_UNPARSED_ARGUMENTS}" STREQUAL "")
        message(FATAL_ERROR "Unknown arguments to cdl_ld")
    endif()

    add_custom_command(
        OUTPUT "${outfile}"
        COMMAND
            ${capdl_linker_tool}
            --arch=${KernelSel4Arch}
            --object-sizes $<TARGET_PROPERTY:object_sizes,FILE_PATH> gen_cdl
            --manifest-in ${CDL_LD_MANIFESTS}
            --elffile ${CDL_LD_ELF}
            --keys ${CDL_LD_KEYS}
            --outfile ${outfile}
        DEPENDS ${CDL_LD_ELF} ${capdl_python} ${CDL_LD_MANIFESTS}
    )
    add_custom_target(${output_target} DEPENDS "${outfile}")
    add_dependencies(${output_target} ${CDL_LD_DEPENDS} object_sizes)

endfunction()

function(cdl_pp manifest_in target)
    cmake_parse_arguments(PARSE_ARGV 2 CDL_PP "" "" "ELF;CFILE;DEPENDS")
    if(NOT "${CDL_PP_UNPARSED_ARGUMENTS}" STREQUAL "")
        message(FATAL_ERROR "Unknown arguments to cdl_pp")
    endif()

    add_custom_command(
        OUTPUT ${CDL_PP_CFILE}
        COMMAND
            ${capdl_linker_tool}
            --arch=${KernelSel4Arch}
            --object-sizes $<TARGET_PROPERTY:object_sizes,FILE_PATH> build_cnode
            --manifest-in=${manifest_in}
            --elffile ${CDL_PP_ELF}
            --ccspace ${CDL_PP_CFILE}
        DEPENDS ${capdl_python} ${manifest_in} object_sizes
    )
    add_custom_target(${target} DEPENDS ${CDL_PP_CFILE})
endfunction()
