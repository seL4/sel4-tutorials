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

ImportCamkes()

# This tutorial is hard coded for the zynq, make sure that was what was selected
if (NOT ("${TUT_BOARD}" STREQUAL "zynq7000"))
    message(FATAL_ERROR "The hello-camkes-timer tutorial is only supported on the zynq7000 board.")
endif()

DeclareCAmkESComponent(Client SOURCES components/Client/src/client.c)
DeclareCAmkESComponent(Timer SOURCES components/Timer/src/timer.c)
DeclareCAmkESComponent(EmptyComponent)

DeclareCAmkESRootserver(hello-camkes-timer.camkes)

GenerateCAmkESRootserver()
/*? declare_task_ordering(['1']) ?*/

/*- filter TaskContent("1", TaskContentType.BEFORE, completion="Booting all finished, dropped to user space") -*//*- endfilter -*/
/*- filter TaskContent("1", TaskContentType.COMPLETED, completion="After the client: wakeup") -*//*- endfilter -*/
/*? macros.cmake_check_script(state) ?*/
