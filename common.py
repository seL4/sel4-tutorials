#!/usr/bin/env python3
#
# Copyright 2018, Data61, CSIRO (ABN 41 687 119 230)
#
# SPDX-License-Identifier: BSD-2-Clause
#

import os
import sys
import sh
import logging

# Define how to configure each platform in terms of arguments passed to cmake
PLAT_CONFIG = {
    'pc99': ['-DTUT_BOARD=pc', '-DTUT_ARCH=x86_64'],
    'zynq7000': ['-DAARCH32=TRUE', '-DTUT_BOARD=zynq7000'],
}

CAMKES_VM_CONFIG = {
    'pc99': ['-DTUT_BOARD=pc', '-DTUT_ARCH=x86_64', "-DFORCE_IOMMU=ON"],
}

ALL_CONFIGS = PLAT_CONFIG.keys()

# Declare each tutorial and the configs they support
TUTORIALS = {
    'hello-world': ALL_CONFIGS,
    'ipc': ALL_CONFIGS,
    'dynamic-1': ALL_CONFIGS,
    'dynamic-2': ALL_CONFIGS,
    'dynamic-3': ALL_CONFIGS,
    'dynamic-4': ALL_CONFIGS,
    'hello-camkes-0': ALL_CONFIGS,
    'hello-camkes-1': ALL_CONFIGS,
    'hello-camkes-2': ALL_CONFIGS,
    'hello-camkes-timer': ['zynq7000'],
    'capabilities': ALL_CONFIGS,
    'untyped': ALL_CONFIGS,
    'mapping': ['pc99'],
    'camkes-vm-linux': ['pc99'],
    'camkes-vm-crossvm': ['pc99'],
    'threads': ALL_CONFIGS,
    'notifications': ['pc99'],
    'mcs': ALL_CONFIGS,
    'interrupts': ['zynq7000'],
    'fault-handlers': ALL_CONFIGS,
}

ALL_TUTORIALS = TUTORIALS.keys()


def get_tutorial_dir():
    '''Return directory containing sel4 tutorials repo'''
    return os.path.dirname(os.path.realpath(__file__))


def get_project_root():
    '''Returns the path to the root directory of this project'''
    # assume default location of this project in projects/sel4-tutorials
    return os.path.join(get_tutorial_dir(), '..', '..')


def _init_build_directory(config, initialised, directory, tute_directory, output=None, config_dict=PLAT_CONFIG):
    args = []
    if not initialised:
        tute_dir = "-DTUTORIAL_DIR=" + os.path.basename(tute_directory)
        args = ['-G', 'Ninja'] + config_dict[config] + [tute_dir] + \
            ["-C", "../projects/sel4-tutorials/settings.cmake"]
    return sh.cmake(args + [tute_directory], _cwd=directory, _out=output, _err=output)


def _init_tute_directory(config, tut, solution, task, directory, output=None):
    if config == "pc99":
        arch = "x86_64"
    elif config == "zynq7000":
        arch = "aarch32"
    with open(os.path.join(directory, ".tute_config"), 'w') as file:
        file.write("set(TUTE_COMMAND \"%s\")" %
                   ';'.join(["PYTHONPATH=${PYTHON_CAPDL_PATH}", "python3", os.path.join(get_tutorial_dir(), "template.py"),
                             "--tut-file", os.path.join(get_tutorial_dir(),
                                                        "tutorials/%s/%s" % (tut, tut)),
                             "--out-dir", "${output_dir}",
                             "--input-files", "${input_files}",
                             "--output-files", "${output_files}",
                             "--arch", arch,
                             "--rt" if tut == "mcs" else "",
                             "--task;%s" % task if task else "",
                             "--solution" if solution else ""]))
    return


def init_directories(config, tut, solution, task, initialised, tute_directory, build_directory, output=None):
    os.chdir(tute_directory)
    _init_tute_directory(config, tut, solution, task, tute_directory, output=sys.stdout)
    os.chdir(build_directory)
    config_dict = None
    if "camkes-vm" in tut:
        config_dict = CAMKES_VM_CONFIG
    else:
        config_dict = PLAT_CONFIG
    return _init_build_directory(config, initialised, build_directory, tute_directory, output, config_dict=config_dict)


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
