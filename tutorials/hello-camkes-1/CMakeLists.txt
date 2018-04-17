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

DeclareTutorialsCAmkESComponent(Client TEMPLATE_SOURCES components/Client/src/client.c)
DeclareTutorialsCAmkESComponent(Echo TEMPLATE_SOURCES components/Echo/src/echo.c)

DeclareCAmkESComponent(EmptyComponent)

DeclareTutorialsCAmkESRootserver(hello-camkes-1.camkes ADL_IS_TEMPLATE
    TEMPLATE_SOURCES interfaces/HelloSimple.camkes
    TUTORIAL_SOURCES components/Client/Client.camkes components/Echo/Echo.camkes
)
