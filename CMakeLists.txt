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

cmake_minimum_required(VERSION 3.8.2)

project(sel4_tutorials C)

# See if a valid tutorial was defined
if (NOT (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tutorials/${TUTORIAL}/CMakeLists.txt"))
    message(FATAL_ERROR "\"${TUTORIAL}\" is not a valid tutorial. Choose one with -DTUTORIAL=<PREFERENCE>")
endif()

if ("${TUTORIAL}" MATCHES "camkes")
    set(CAMKES TRUE)
else ()
    set(CAMKES FALSE)
endif()

# Deal with the top level target-triplet variables.
if (NOT TUT_BOARD)
    message(FATAL_ERROR "Please select a board to compile for. Choose either pc or zynq7000\n\t`-DTUT_BOARD=<PREFERENCE>`")
endif()

# Set arch and board specific kernel parameters here.
if (${TUT_BOARD} STREQUAL "pc")
    set(KernelArch "x86" CACHE STRING "" FORCE)
    if (${TUT_ARCH} STREQUAL "ia32")
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

# For the tutorials that do initialize the plat support serial printing they still
# just want to use the kernel debug putchar if it exists
set(LibSel4PlatSupportUseDebugPutChar true CACHE BOOL "" FORCE)

# Just let the regular abort spin without calling DebugHalt to prevent needless
# confusing output from the kernel for a tutorial
set(LibSel4MuslcSysDebugHalt FALSE CACHE BOOL "" FORCE)

if (CAMKES)
    set(CapDLLoaderMaxObjects 20000 CACHE STRING "" FORCE)
    set(KernelRootCNodeSizeBits 17 CACHE STRING "" FORCE)
    set(KernelNumDomains 1 CACHE STRING "" FORCE)
endif()

# We must build the debug kernel because the tutorials rely on seL4_DebugPutChar
# and they don't initialize a platsupport driver.
ApplyCommonReleaseVerificationSettings(FALSE FALSE)

# We will attempt to generate a simulation script, so try and generate a simulation
# compatible configuration
ApplyCommonSimulationSettings()

ApplyData61ElfLoaderSettings()

# Ensure we have the python script for processing the templates
RequireFile(TemplateTool "template.py")
# Force cmake to rerun if the template tool changes
set_property(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${TemplateTool}")

# Save the tutorial directory as this is where we want to define all our tutorial paths to
# be relative to.
set(tutorial_base_dir "${CMAKE_CURRENT_SOURCE_DIR}/tutorials")

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

# Copies a tutorial source file into the destination location.
# `parent_dest` is the name of a variable where the full path of the outputted file will be placed
# `name` is the filename relative to the current source directory but without any '.template' or '.source' extension
#   This name then becomes the name that the user of the tutorial will see and edit
# `extension` is any extension on the `name` to provide the full file path. By convention templated files
#   '.template' and non-templated files have '.source'. The reason for always having an extension is to
#   prevent them being accidentally used from the source directory instead of destination version that the
#   user will be editing
# `is_template` indicates whether the file should be put through the template preprocessor or not
function(CopyTutorialSource parent_dest name extension is_template)
    if (BUILD_SOLUTIONS)
        set(args "--solution")
    endif()
    file(RELATIVE_PATH tut_dir "${tutorial_base_dir}" "${CMAKE_CURRENT_LIST_DIR}")
    set(dest_folder "${tut_dir}")
    set(temp_folder "temp/${dest_folder}")
    set(template "${CMAKE_CURRENT_SOURCE_DIR}/${name}${extension}")
    set(dest "${CMAKE_BINARY_DIR}/${dest_folder}/${name}")
    set(temp "${CMAKE_BINARY_DIR}/${temp_folder}/${name}.temp")
    set(orig "${CMAKE_BINARY_DIR}/${temp_folder}/${name}.orig")
    set_property(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${template}")
    # Generate the temp file. We do this unconditionally as it should be cheap and our only
    # dependency on the TemplateTool is by requesting that this whole script re-runs
    if (is_template)
        EnsureDir("${temp}")
        execute_process(
            COMMAND "${TemplateTool}" ${args}
            RESULT_VARIABLE exit_status
            INPUT_FILE "${template}"
            OUTPUT_FILE "${temp}"
            ERROR_VARIABLE error_message
        )
        if (NOT ("${exit_status}" EQUAL 0))
            message(FATAL_ERROR "Template parsing of ${template} failed with code ${exit_status} and stderr: \"${error_message}\"")
        endif()
    else()
        Copy("${template}" "${temp}")
    endif()
    # Check if the original is the same as what we generated
    if (EXISTS "${orig}")
        execute_process(
            COMMAND diff -q "${orig}" "${temp}"
            RESULT_VARIABLE exit_status
            OUTPUT_QUIET
            ERROR_QUIET
        )
        if (NOT ("${exit_status}" EQUAL 0))
            message(FATAL_ERROR "Dependent tutorial file ${template} has been modified, unable to distinguish "
                                "between local changes to it and local version at ${dest}, please regenerate "
                                "tutorial")
        endif()
    endif()
    # Move and make the original
    Rename("${temp}" "${orig}")
    # Determine the relative portion of the destination from what we believe the
    # users working directory (CMAKE_BINARY_DIR) to be
    file(RELATIVE_PATH rel "${CMAKE_BINARY_DIR}" "${dest}")
    # If the destination does not exist then make it, otherwise do not overwrite
    # as the user may have made changes to it
    if(NOT (EXISTS "${dest}"))
        Copy("${orig}" "${dest}")
    endif()
    if(is_template)
        set_property(GLOBAL APPEND PROPERTY template_files "${rel}")
    else()
        set_property(GLOBAL APPEND PROPERTY companion_files "${rel}")
    endif()
    set(${parent_dest} "${dest}" PARENT_SCOPE)
endfunction(CopyTutorialSource)

# Wrapper around `add_executable` that understands two additional list arguments in the form of
# TEMPLATE_SOURCES and TUTORIAL_SOURCES. These are passed through CopyTutorialSource and the resulting
# files are given to `add_executable` along with any other additional unparsed arguments
function(add_tutorials_executable name)
    cmake_parse_arguments(PARSE_ARGV 1 ADD_TUT_EXE "" "" "TEMPLATE_SOURCES;TUTORIAL_SOURCES")
    foreach(source IN LISTS ADD_TUT_EXE_TUTORIAL_SOURCES)
        CopyTutorialSource(dest "${source}" ".tutorial" FALSE)
        list(APPEND sources "${dest}")
    endforeach()
    foreach(source IN LISTS ADD_TUT_EXE_TEMPLATE_SOURCES)
        CopyTutorialSource(dest "${source}" ".template" TRUE)
        list(APPEND sources "${dest}")
    endforeach()
    add_executable(${name} ${ADD_TUT_EXE_UNPARSED_ARGUMENTS} ${sources})
endfunction(add_tutorials_executable)

# Wrapper around `DeclareCAmkESComponent` that understands two additional list arguments in the form of
# TEMPLATE_SOURCES and TUTORIAL_SOURCES. These are passed through CopyTutorialSource and the resulting
# files are given to `DeclareCAmkESComponent` along with any other additional unparsed arguments
function(DeclareTutorialsCAmkESComponent name)
    cmake_parse_arguments(PARSE_ARGV 1 ADD_TUT_COMPONENT "" "" "TUTORIAL_SOURCES;TEMPLATE_SOURCES")
    foreach(source IN LISTS ADD_TUT_COMPONENT_TUTORIAL_SOURCES)
        CopyTutorialSource(dest "${source}" ".tutorial" FALSE)
        list(APPEND sources "${dest}")
    endforeach()
    foreach(source IN LISTS ADD_TUT_COMPONENT_TEMPLATE_SOURCES)
        CopyTutorialSource(dest "${source}" ".template" TRUE)
        list(APPEND sources "${dest}")
    endforeach()
    DeclareCAmkESComponent(${name} ${ADD_TUT_COMPONENT_UNPARSED_ARGUMENTS} SOURCES ${sources})
endfunction(DeclareTutorialsCAmkESComponent)

# Wrapper around `DeclareCAmkESRootserver` that takes additional lists of `TUTORIAL_SOURCES` and `TEMPLATE_SOURCES`
# to process and move into the tutorial build directory. Unlike other wrappers the process files are not
# passed through to the invocation of `DeclareCAmkESRootserver` but the intention is that the ADL file
# that is processed and passed to `DeclareCAmkESRootserver` may refer to other files and these should be
# specified in these lists.
function(DeclareTutorialsCAmkESRootserver adl)
    cmake_parse_arguments(PARSE_ARGV 1 ADD_TUT_CAMKES "ADL_IS_TUTORIAL;ADL_IS_TEMPLATE" "" "TUTORIAL_SOURCES;TEMPLATE_SOURCES")
    if (ADD_TUT_CAMKES_ADL_IS_TUTORIAL)
        CopyTutorialSource(adl_dest "${adl}" ".tutorial" FALSE)
    elseif (ADD_TUT_CAMKES_ADL_IS_TEMPLATE)
        CopyTutorialSource(adl_dest "${adl}" ".template" TRUE)
    endif()
    foreach(source IN LISTS ADD_TUT_CAMKES_TUTORIAL_SOURCES)
        CopyTutorialSource(dest "${source}" ".tutorial" FALSE)
    endforeach()
    foreach(source IN LISTS ADD_TUT_CAMKES_TEMPLATE_SOURCES)
        CopyTutorialSource(dest "${source}" ".template" TRUE)
    endforeach()
    DeclareCAmkESRootserver("${adl_dest}")
endfunction(DeclareTutorialsCAmkESRootserver)

if (CAMKES)
    # We are including camkes from a non standard location and need to give some paths
    # This is a very non standard way of using CAmkES as normally you would use the
    # default top level CMakeLists.txt from CAmkES, but as we wish to switch CAmkES on and
    # off this is not possible, hence we have to do this mangling here ourselves.
    set(PYTHON_CAPDL_PATH "${CMAKE_SOURCE_DIR}/projects/camkes/capdl/python-capdl-tool")
    set(CAPDL_TOOL_SOURCE_PATH "${CMAKE_SOURCE_DIR}/projects/camkes/capdl/capDL-tool")
    find_program(TPP_TOOL tpp PATHS "${CMAKE_SOURCE_DIR}/tools/camkes/tools")
    include("${CMAKE_SOURCE_DIR}/tools/camkes/camkes.cmake")
    add_subdirectory("${CMAKE_SOURCE_DIR}/projects/camkes/capdl/capdl-loader-app" capdl-loader-app)
    add_subdirectory("${CMAKE_SOURCE_DIR}/projects/camkes/global-components" global-components)
    add_subdirectory("${CMAKE_SOURCE_DIR}/tools/camkes/libsel4camkes" libsel4camkes)
endif()

add_subdirectory(tutorials/${TUTORIAL})

if (CAMKES)
    GenerateCAmkESRootserver()
else()
    if (NOT (TARGET ${TUTORIAL}))
        message(FATAL_ERROR "bug: ${TUTORIAL} did not define a target with the same name")
    endif()

    DeclareRootserver(${TUTORIAL})
endif()

GenerateSimulateScript()

# Print out any information regarding to sources that were constructed
# whilst processing the tutorial. This is guarded by a config option so that
# it can be toggled externally to prevent the instructions printing out every
# time cmake runs.
if (TUTORIALS_PRINT_INSTRUCTIONS)
    get_property(template_files GLOBAL PROPERTY template_files)
    get_property(companion_files GLOBAL PROPERTY companion_files)
    if (NOT ("${companion_files}" STREQUAL ""))
        message("The following files do not require modifications but are copied to provide context:")
        foreach(file IN LISTS companion_files)
            message("\t${file}")
        endforeach()
    endif()
    if (NOT ("${template_files}" STREQUAL ""))
        if (BUILD_SOLUTIONS)
            message("Tutorial solutions placed in:")
        else()
            message("The following files need to be modified to complete the tutorial")
        endif()
        foreach(file IN LISTS template_files)
            message("\t${file}")
        endforeach()
    endif()
endif()
