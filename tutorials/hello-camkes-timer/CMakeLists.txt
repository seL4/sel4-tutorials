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

cmake_minimum_required(VERSION 3.7.2)

project(hello-camkes-timer C)

# This tutorial is hard coded for the zynq, make sure that was what was selected
if (NOT ("${TUT_BOARD}" STREQUAL "zynq7000"))
    message(FATAL_ERROR "The hello-camkes-timer tutorial is only supported on the zynq7000 board.")
endif()

DeclareTutorialsCAmkESComponent(Client TUTORIAL_SOURCES components/Client/src/client.c)
DeclareTutorialsCAmkESComponent(Timer TEMPLATE_SOURCES components/Timer/src/timer.c)
DeclareCAmkESComponent(EmptyComponent)

DeclareTutorialsCAmkESRootserver(hello-camkes-timer.camkes ADL_IS_TEMPLATE
    TUTORIAL_SOURCES interfaces/timer.camkes components/Client/Client.camkes
        components/Timer/Timer.camkes
)
