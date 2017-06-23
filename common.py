#!/usr/bin/env python
#
# Copyright 2017, Data61
# Commonwealth Scientific and Industrial Research Organisation (CSIRO)
# ABN 41 687 119 230.
#
# This software may be distributed and modified according to the terms of
# the BSD 2-Clause license. Note that NO WARRANTY is provided.
# See "LICENSE_BSD2.txt" for details.
#
# @TAG(D61_BSD)
#

import re
import os
import sys
import logging

ARCHS = ['ia32', 'arm']
PLATS = ['ia32', 'imx6', 'zynq7000']

BUILD_CONFIG_PAT = r'(?P<prefix>%s)_(?P<name>.*)_defconfig' % "|".join(PLATS)
BUILD_CONFIG_RE = re.compile(BUILD_CONFIG_PAT)

def config_filename_to_parts(filename):
    '''Return a tuple (prefix, name) for a given build config filename'''
    m = BUILD_CONFIG_RE.match(filename)
    if m is not None:
        prefix = m.group('prefix')
        name = m.group('name')
        return prefix, name
    else:
        raise Exception("invalid build config filename: %s" % filename)

def config_filename_from_parts(prefix, name):
    '''Return a build config filename for a given prefix and name'''
    return "%s_%s_defconfig" % (prefix, name)

def get_tutorial_dir():
    '''Return directory containing sel4 tutorials repo'''
    return os.path.dirname(os.path.realpath(__file__))

def get_project_root():
    '''Returns the path to the root directory of this project'''
    # assume default location of this project in projects/sel4-tutorials
    return os.path.join(get_tutorial_dir(), '..', '..')

def get_config_dir():
    '''Returns the path to the "configs" dir inside the root directory of this project'''
    return os.path.realpath(os.path.join(get_project_root(), 'configs'))

def get_tutorial_type():
    '''Returns a string identifying which tutorial environment we are currently
       in, based on the config dir symlink.
    '''
    config_path = get_config_dir()

    if not os.path.isdir(config_path):
        return None

    config_path_end = os.path.split(config_path)[-1]
    if config_path_end == 'configs-camkes':
        return 'CAmkES'
    elif config_path_end == 'configs-sel4':
        return 'seL4'
    else:
        raise Exception("unexpected build config path: %s" % config_path)

def setup_logger(name):
    logger = logging.getLogger(name)
    ch = logging.StreamHandler(sys.stdout)
    formatter = logging.Formatter('%(message)s')
    ch.setFormatter(formatter)
    logger.addHandler(ch)
    return logger
