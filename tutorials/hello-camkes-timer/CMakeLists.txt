#
# Copyright 2018, Data61, CSIRO (ABN 41 687 119 230)
#
# SPDX-License-Identifier: BSD-2-Clause
#

include(${SEL4_TUTORIALS_DIR}/settings.cmake)
sel4_tutorials_regenerate_tutorial(${CMAKE_CURRENT_SOURCE_DIR})

cmake_minimum_required(VERSION 3.7.2)

project(hello-camkes-timer C ASM)

find_package(camkes-tool REQUIRED)
camkes_tool_setup_camkes_build_environment()

# This tutorial is hard coded for the zynq, make sure that was what was selected
if(NOT ("${TUT_BOARD}" STREQUAL "zynq7000"))
    message(FATAL_ERROR "The hello-camkes-timer tutorial is only supported on the zynq7000 board.")
endif()

set(CAmkESDTS ON CACHE BOOL "" FORCE)

DeclareCAmkESComponent(Client SOURCES components/Client/src/client.c)
DeclareCAmkESComponent(
    Timer
    SOURCES
    components/Timer/src/timer.c
    ${SEL4_TUTORIALS_DIR}/zynq_timer_driver/src/driver.c
    INCLUDES
    ${SEL4_TUTORIALS_DIR}/zynq_timer_driver/include
)
DeclareCAmkESComponent(
    TimerDTB
    SOURCES
    components/TimerDTB/src/timerdtb.c
    ${SEL4_TUTORIALS_DIR}/zynq_timer_driver/src/driver.c
    INCLUDES
    ${SEL4_TUTORIALS_DIR}/zynq_timer_driver/include
)
DeclareCAmkESComponent(EmptyComponent)

DeclareCAmkESRootserver(hello-camkes-timer.camkes)

GenerateCAmkESRootserver()
/*? declare_task_ordering(['1']) ?*/

/*- filter TaskContent("1", TaskContentType.BEFORE, completion="Booting all finished, dropped to user space") -*//*- endfilter -*/
/*- filter TaskContent("1", TaskContentType.COMPLETED, completion="After the client: wakeup") -*//*- endfilter -*/
/*? macros.cmake_check_script(state) ?*/
