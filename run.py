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

# Script for building and running tutorials
# This file can be executed by itself, or used as a module by other scripts.

import os
import sys
import argparse
import sh
import logging

import common

logger = logging.getLogger(__name__)

PLATS = ['pc99', 'imx31']

CONFIG_PREFIX_TO_ARCH = {
    'ia32': 'ia32',
    'arm': 'arm',
}
CONFIG_PREFIX_TO_PLAT = {
    'ia32': 'pc99',
    'arm': 'imx31',
}
ARCH_TO_DEFAULT_PLAT = {
    'ia32': 'pc99',
    'arm': 'imx31',
}
PLAT_TO_QEMU_BIN = {
    'pc99': 'qemu-system-i386',
    'imx31': 'qemu-system-arm',
}
PLAT_TO_QEMU_ARGS = {
    'pc99': ['-nographic', '-m', '512', '-cpu', 'Haswell'],
    'imx31': ['-nographic', '-M', 'kzm'],
}

def list_configs():
    '''Lists names of build config files'''
    return os.listdir(common.get_config_dir())

def list_names():
    '''Lists names of apps'''
    return set(name for name, _, _ in map(config_file_to_info, list_configs()))

def list_names_for_target(target_arch, target_plat):
    '''Lists names of apps for which there exist a build config for a specified
       architecture and platform
    '''
    return (name for name, arch, plat in map(config_file_to_info, list_configs())
            if arch == target_arch and plat == target_plat)

def config_file_to_info(filename):
    '''Returns a (name, arch, plat) tuple fora given build config file name'''
    prefix, name = common.config_filename_to_parts(filename)
    return name, CONFIG_PREFIX_TO_ARCH[prefix], CONFIG_PREFIX_TO_PLAT[prefix]

def arch_to_config_file(arch, name):
    '''Returns a build config file name for a given (name, arch, plat) tuple'''
    return common.config_filename_from_parts(arch, name)

def check_config(arch, plat, name):
    '''Returns True if a given arch, plat, name corresponds to an existing
       build config file. Otherwise returns false and prints a list of
       valid names.
    '''
    names = list(list_names_for_target(arch, plat))

    if name not in names:
        logger.error("No tutorial named \"%s\" for %s/%s." % (name, arch, plat))
        logger.error("Tutorials for %s/%s: %s" % (arch, plat, ", ".join(names)))

        archs_with_tutorial = []
        for a in ARCHS:
            if a != arch:
                other_plat = ARCH_TO_DEFAULT_PLAT[a]
                other_names = list(list_names_for_target(a, other_plat))
                if name in other_names:
                    archs_with_tutorial.append(a)

        logger.error("Architectures with \"%s\" tutorial: %s" % \
            (name, ", ".join(archs_with_tutorial)))

        return False

    return True

def make_parser():
    '''Creates a parser for the tutorial runner. The arch agrument is a string
       specifying the architecture. Valid architecture strings are "ia32" and "arm".
    '''
    names = list_names()
    parser = argparse.ArgumentParser(description='%s Tutorial Runner' % common.get_tutorial_type())
    parser.add_argument('name', type=str, help='name of tutorial to run', choices=list(names))
    parser.add_argument('-a', '--arch', dest='arch', type=str,
                        help='architecture to build for and emulate', choices=common.ARCHS, required=True)
    parser.add_argument('-j', '--jobs', dest='jobs', type=int,
                        help='number of jobs to use while building', default=1)
    parser.add_argument('-p', '--plat', dest='plat', type=str,
                        help='platform to build for and emulate', choices=PLATS)

    return parser

def runcmd(cmd, *args):
    '''Run a command using sh, logging the output'''
    for line in getattr(sh, cmd)(*args, _err_to_out=True, _iter=True):
        logger.info(line.rstrip())

def build(arch, plat, name, jobs):
    '''Builds the specified tutorial'''
    runcmd('make', 'clean')
    runcmd('make', arch_to_config_file(arch, name))
    runcmd('make', 'silentoldconfig')
    runcmd('make', '-j%d' % jobs)

def get_qemu_image_args(arch, plat, name):
    '''Return a list of arguments for qemu to specify which image to run'''

    if common.get_tutorial_type() == 'CAmkES':
        app_image = 'images/capdl-loader-experimental-image-%s-%s' % (arch, plat)
    else:
        app_image = 'images/%s-image-%s-%s' % (name, arch, plat)

    if arch == 'ia32':
        return ['-kernel', 'images/kernel-ia32-pc99', '-initrd', app_image]
    else:
        return ['-kernel', app_image]

def run(arch, plat, name):
    '''Runs the specified app in qemu'''

    qemu = PLAT_TO_QEMU_BIN[plat]
    qemu_args = PLAT_TO_QEMU_ARGS[plat]

    try:
        runcmd(qemu, *(qemu_args + get_qemu_image_args(arch, plat, name)))
    except sh.CommandNotFound:
        raise Exception("%s is not installed" % qemu)

def setup_logger():
    logger = logging.getLogger(__name__)
    ch = logging.StreamHandler()
    formatter = logging.Formatter('%(message)s')
    ch.setFormatter(formatter)
    logger.addHandler(ch)
    logger.setLevel(logging.INFO)

def main(argv):
    setup_logger()
    args = make_parser().parse_args(argv)

    if args.plat is None:
        args.plat = ARCH_TO_DEFAULT_PLAT[args.arch]

    if not check_config(args.arch, args.plat, args.name):
        return -1

    build(args.arch, args.plat, args.name, args.jobs)

    run(args.arch, args.plat, args.name)

    return 0

if __name__ == '__main__':
    try:
        sys.exit(main(sys.argv[1:]))
    except KeyboardInterrupt:
        sys.exit(130)
