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
import argparse
import subprocess

ARCHS = ['ia32', 'arm']
PLATS = ['pc99', 'imx31', 'sabre']

CONFIG_PREFIX_TO_ARCH = {
    'ia32': 'ia32',
    'arm': 'arm',
    'sabre': 'arm',
}
CONFIG_PREFIX_TO_PLAT = {
    'ia32': 'pc99',
    'arm': 'imx31',
    'sabre': 'sabre',
}
ARCH_TO_DEFAULT_PLAT = {
    'ia32': 'pc99',
    'arm': 'imx31',
}
PLAT_TO_QEMU_BIN = {
    'pc99': 'qemu-system-i386',
    'imx31': 'qemu-system-arm',
    'sabre': 'qemu-system-arm',
}
PLAT_TO_QEMU_ARGS = {
    'pc99': ['-nographic', '-m', '512', '-cpu', 'Haswell'],
    'imx31': ['-nographic', '-M', 'kzm'],
    'sabre': ['-nographic', '-M', 'sabrelite', '-machine', 'sabrelite', '-m', 'size=1024M',
              '-serial', 'null', '-serial', 'mon:stdio', '-'],
}

BUILD_CONFIG_PAT = r'(?P<prefix>%s)_(?P<name>.*)_defconfig' % "|".join(ARCHS)
BUILD_CONFIG_RE = re.compile(BUILD_CONFIG_PAT)

def get_project_root():
    script_dir = os.path.dirname(os.path.realpath(__file__))
    # assume default location of this project in projects/sel4-tutorials
    return os.path.join(script_dir, '..', '..')

def get_config_dir():
    return os.path.realpath(os.path.join(get_project_root(), 'configs'))

def list_configs():
    return os.listdir(get_config_dir())

def list_names():
    return set(name for name, _, _ in map(config_file_to_info, list_configs()))

def list_names_for_target(target_arch, target_plat):
    return (name for name, arch, plat in map(config_file_to_info, list_configs())
            if arch == target_arch and plat == target_plat)

def config_file_to_info(filename):
    m = BUILD_CONFIG_RE.match(filename)
    if m is not None:
        prefix = m.group('prefix')
        name = m.group('name')
        return (name, CONFIG_PREFIX_TO_ARCH[prefix], CONFIG_PREFIX_TO_PLAT[prefix])
    else:
        raise Exception("unrecognized build config %s" % filename)

def info_to_config_file(arch, plat, name):
    if plat == 'sabre':
        return 'sabre_%s_defconfig' # XXX special case for sabre platform

    return '%s_%s_defconfig' % (arch, name)

def check_config(arch, plat, name):
    names = list(list_names_for_target(arch, plat))

    if name not in names:
        print("No tutorial named \"%s\" for %s/%s. Valid tutorials: %s" %
              (name, arch, plat, ", ".join(names)))
        return False

    return True

def get_tutorial_type():
    config_path = get_config_dir()
    config_path_end = os.path.split(config_path)[-1]
    if config_path_end == 'configs-camkes':
        return 'CAmkES'
    elif config_path_end == 'configs-sel4':
        return 'seL4'
    elif config_path_end == 'configs-sel4-rt':
        return 'seL4-rt'
    else:
        raise Exception("unexpected build config path: %s" % config_path)

def make_parser():
    '''Creates a parser for the tutorial runner. The arch agrument is a string
       specifying the architecture. Valid architecture strings are "ia32" and "arm".
    '''
    names = list_names()
    parser = argparse.ArgumentParser(description='%s Tutorial Runner' % get_tutorial_type())
    parser.add_argument('name', type=str, help='name of tutorial to run', choices=list(names))
    parser.add_argument('-a', '--arch', dest='arch', type=str,
                        help='architecture to build for and emulate', choices=ARCHS, required=True)
    parser.add_argument('-j', '--jobs', dest='jobs', type=int,
                        help='number of jobs to use while building', default=1)
    parser.add_argument('-p', '--plat', dest='plat', type=str,
                        help='platform to build for and emulate', choices=PLATS)

    return parser

def build(arch, plat, name, jobs):
    subprocess.call(['make', 'clean'])
    subprocess.call(['make', info_to_config_file(arch, plat, name)])
    subprocess.call(['make', 'silentoldconfig'])
    subprocess.call(['make', '-j%d' % jobs])

def get_qemu_image_args(arch, plat, name):

    if get_tutorial_type() == 'CAmkES':
        app_image = 'images/capdl-loader-experimental-image-%s-%s' % (arch, plat)
    else:
        app_image = 'images/%s-image-%s-%s' % (name, arch, plat)

    if arch == 'ia32':
        return ['-kernel', 'images/kernel-ia32-pc99', '-initrd', app_image]
    else:
        return ['-kernel', app_image]

def run(arch, plat, name):
    qemu = PLAT_TO_QEMU_BIN[plat]
    qemu_args = PLAT_TO_QEMU_ARGS[plat]

    subprocess.call([qemu] + qemu_args + get_qemu_image_args(arch, plat, name))

def main(argv):
    args = make_parser().parse_args(argv)

    if args.plat is None:
        args.plat = ARCH_TO_DEFAULT_PLAT[args.arch]

    if not check_config(args.arch, args.plat, args.name):
        return -1

    build(args.arch, args.plat, args.name, args.jobs)

    run(args.arch, args.plat, args.name)

    return 0

if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
