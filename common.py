#!/usr/bin/env python
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

import os
import sys
import sh
import logging

# Define how to configure each platform in terms of arguments passed to cmake
PLAT_CONFIG = {
    'pc99': ['-DTUT_BOARD=pc', '-DTUT_ARCH=x86_64'],
    'imx6': ['-DAARCH32=TRUE', '-DTUT_BOARD=imx6'],
    'zynq7000': ['-DAARCH32=TRUE', '-DTUT_BOARD=zynq7000'],
}

ALL_CONFIGS = PLAT_CONFIG.keys()

# Declare each tutorial and the configs they support
TUTORIALS = {
    'hello-1': ALL_CONFIGS,
    'hello-2': ALL_CONFIGS,
    'hello-2-nolibs': ALL_CONFIGS,
    'hello-3': ALL_CONFIGS,
    'hello-3-nolibs': ALL_CONFIGS,
    'hello-4': ALL_CONFIGS,
    'hello-timer': ALL_CONFIGS,
    'hello-camkes-0': ALL_CONFIGS,
    'hello-camkes-1': ALL_CONFIGS,
    'hello-camkes-2': ALL_CONFIGS,
    'hello-camkes-timer': ['zynq7000'],
}

ALL_TUTORIALS = TUTORIALS.keys()

def get_tutorial_dir():
    '''Return directory containing sel4 tutorials repo'''
    return os.path.dirname(os.path.realpath(__file__))

def get_project_root():
    '''Returns the path to the root directory of this project'''
    # assume default location of this project in projects/sel4-tutorials
    return os.path.join(get_tutorial_dir(), '..', '..')

def init_build_directory(config, tut, solution, directory):
    tut_arg = "-DTUTORIAL=" + tut
    args = ['-DCMAKE_TOOLCHAIN_FILE=../kernel/gcc.cmake', '-G', 'Ninja'] + PLAT_CONFIG[config] + [tut_arg];
    if solution:
        args = args + ["-DBUILD_SOLUTIONS=TRUE"]
    result = sh.cmake(args + ['..'], _cwd = directory)
    if result.exit_code != 0:
        return result
    sh.cmake(['..'], _cwd = directory)
    if result.exit_code != 0:
        return result
    sh.cmake(['..'], _cwd = directory)
    if result.exit_code != 0:
        return result
    return sh.cmake(['..'], _cwd = directory)

def set_log_level(verbose, quiet):
    if verbose:
        logging.basicConfig(level=logging.DEBUG)
    elif quiet:
        logging.basicConfig(level=logging.ERROR)
    else:
        logging.basicConfig(level=logging.INFO)

def setup_logger(name):
    logger = logging.getLogger(name)
    ch = logging.StreamHandler(sys.stdout)
    formatter = logging.Formatter('%(message)s')
    ch.setFormatter(formatter)
    logger.addHandler(ch)
    return logger
