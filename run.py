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
# @TAG(DATA61_BSD)
#

# Script for building and running tutorials
# This file can be executed by itself, or used as a module by other scripts.

import os
import sys
import argparse
import logging
import subprocess

import sh
import common

logger = common.setup_logger(__name__)

PLATS = ['pc99', 'imx31', 'imx6', 'zynq7000']

CONFIG_PREFIX_TO_ARCH = {
    'pc99': 'ia32',
    'arm': 'arm',
    'zynq7000': 'arm',
    'kzm': 'arm',
}
CONFIG_PREFIX_TO_PLAT = {
    'pc99': 'pc99',
    'kzm': 'imx31',
    'zynq7000': 'zynq7000',
}
ARCH_TO_DEFAULT_PLAT = {
    'ia32': 'pc99',
    'arm': 'zynq7000',
}

def list_configs():
    '''Lists names of build config files'''
    try:
        return os.listdir(common.get_config_dir())
    except:
        return []

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

def plat_to_config_file(plat, name):
    '''Returns a build config file name for a given (name, arch, plat) tuple'''
    return common.config_filename_from_parts(plat, name)

def check_config(arch, plat, name):
    '''Returns True if a given arch, plat, name corresponds to an existing
       build config file. Otherwise returns false and prints a list of
       valid names.
    '''
    names = list(list_names_for_target(arch, plat))

    if name not in names and name != 'all':
        logger.error("No tutorial named \"%s\" for %s/%s." % (name, arch, plat))
        logger.error("Tutorials for %s/%s: %s" % (arch, plat, ", ".join(names)))

        archs_with_tutorial = []
        for a in common.ARCHS:
            if a != arch:
                other_plat = ARCH_TO_DEFAULT_PLAT[a]
                other_names = list(list_names_for_target(a, other_plat))
                if name in other_names:
                    archs_with_tutorial.append(a)

        logger.error("Architectures with \"%s\" tutorial: %s" % \
            (name, ", ".join(archs_with_tutorial)))

        return False

    return True

def get_description():
    return '%s Tutorial Runner' % common.get_tutorial_type()

def add_arguments(parser):
    names = list_names()
    parser.add_argument('name', type=str, help='name of tutorial to run', choices=list(names)+['all'])
    parser.add_argument('-a', '--arch', dest='arch', type=str,
                        help='architecture to build for and emulate', choices=common.ARCHS, required=True)
    parser.add_argument('-j', '--jobs', dest='jobs', type=int,
                        help='number of jobs to use while building', default=1)
    parser.add_argument('-p', '--plat', dest='plat', type=str,
                        help='platform to build for and emulate', choices=PLATS)

def process_output(line):
    print(line.rstrip())

def build(arch, plat, name, jobs):
    '''Builds the specified tutorial'''
    make = sh.make.bake(_out=process_output, _err=process_output)
    logger.info('make clean')
    make.clean()
    config = plat_to_config_file(plat, name)
    logger.info('make ' + config)
    make(config)
    logger.info('make silentoldconfig')
    make.silentoldconfig()
    logger.info('make -j' + str(jobs))
    make('-j%d' % jobs)

def run():
    make = sh.make.bake(_out=process_output, _err=process_output)
    logger.info('make simulate')
    logger.info('Use Ctrl-C to exit')
    make.simulate()

def make_parser():
    parser = argparse.ArgumentParser(description=get_description())
    add_arguments(parser)
    return parser

def handle_run(args, loglevel=logging.INFO):
    logger.setLevel(loglevel)
    if args.plat is None:
        args.plat = ARCH_TO_DEFAULT_PLAT[args.arch]

    if not check_config(args.arch, args.plat, args.name):
        return -1

    if args.name == 'all':
        for name in list_names():
            build(args.arch, args.plat, name, args.jobs)
            run()
    else:
        build(args.arch, args.plat, args.name, args.jobs)
        run()

def add_sub_parser_run(subparsers):
    '''Creates a parser for the tutorial runner. The arch agrument is a string
       specifying the architecture. Valid architecture strings are "ia32" and "arm".
    '''
    parser = subparsers.add_parser('run', description=get_description(), help='run the tutorials in the current environment')
    add_arguments(parser)
    parser.set_defaults(func=handle_run)
    return parser


def main(argv):
    args = make_parser().parse_args(argv)
    handle_run(args)

    return 0

if __name__ == '__main__':
    try:
        sys.exit(main(sys.argv[1:]))
    except KeyboardInterrupt:
        sys.exit(130)
