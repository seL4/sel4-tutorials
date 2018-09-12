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

project(hello-camkes-1 C)

ImportCamkes()

DeclareCAmkESComponent(Client SOURCES components/Client/src/client.c)
DeclareCAmkESComponent(Echo SOURCES components/Echo/src/echo.c)

DeclareCAmkESComponent(EmptyComponent)

DeclareCAmkESRootserver(hello-camkes-1.camkes)

GenerateCAmkESRootserver()
/*? declare_task_ordering(['1']) ?*/
/*- filter TaskContent("1", TaskContentType.BEFORE, completion="Caught cap fault in send phase") -*//*- endfilter -*/
/*- filter TaskContent("1", TaskContentType.COMPLETED, completion="Component echo saying: hello world") -*//*- endfilter -*/
/*? macros.cmake_check_script(state) ?*/
