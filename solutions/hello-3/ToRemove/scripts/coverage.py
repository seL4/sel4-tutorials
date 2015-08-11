#!/usr/bin/env python
#
# Copyright 2014, NICTA
#
# This software may be distributed and modified according to the terms of
# the BSD 2-Clause license. Note that NO WARRANTY is provided.
# See "LICENSE_BSD2.txt" for details.
#
# @TAG(NICTA_BSD)
#

#
# To use this coverage script, run a binary with qemu, passing the
# "-singlestep" and "-d exec" options. You will probably also want to use -D to
# prevent the log going to stderr. Note that this logging only works on newer
# versions of qemu.
#
# coverage.py staging/arm/imx31/kernel.elf /tmp/qemu.log --functions --objdump | less -R
#

import sys
import os
import re
import argparse
from subprocess import Popen, PIPE

class Colors(object):
    def __init__(self, use_color):
        c = {}
        c['NORMAL'] = '\033[0m'
        c['BLACK'] = '\033[0;30m'
        c['DARK_RED'] = '\033[0;31m'
        c['DARK_GREEN'] = '\033[0;32m'
        c['DARK_YELLOW'] = '\033[0;33m'
        c['DARK_BLUE'] = '\033[0;34m'
        c['DARK_MAGENTA'] = '\033[0;35m'
        c['DARK_CYAN'] = '\033[0;36m'
        c['GREY'] = '\033[0;37m'
        c['LIGHT_GREY'] = '\033[1;30m'
        c['LIGHT_RED'] = '\033[1;31m'
        c['LIGHT_GREEN'] = '\033[1;32m'
        c['LIGHT_YELLOW'] = '\033[1;33m'
        c['LIGHT_BLUE'] = '\033[1;34m'
        c['LIGHT_MAGENTA'] = '\033[1;35m'
        c['LIGHT_CYAN'] = '\033[1;36m'
        c['WHITE'] = '\033[1;37m'

        if use_color:
            self.c = c
        else:
            self.c = dict([(x, '') for x in c.keys()])

    def __getattr__(self, name):
        if name in self.c:
            return self.c[name]
        else:
            return object.__getattribute__(self, name)

def get_tool(toolname):
    default_prefix = 'arm-none-eabi-'

    return os.environ.get('TOOLPREFIX', default_prefix) + toolname

def main():
    parser = argparse.ArgumentParser(
        description='Generate coverage information of a binary.')
    parser.add_argument('kernel_elf_filename', metavar='<kernel ELF>',
        type=str, help='The kernel ELF file used for the log.')
    parser.add_argument('coverage_filename', metavar='<qemu log>',
        type=str, help='The qemu logfile containing the instruction trace.')
    parser.add_argument('--functions', action='store_true',
        help='Produce a summary of the functions covered.')
    parser.add_argument('--objdump', action='store_true',
        help='Produce an objdump with coverage information.')
    parser.add_argument('--no-color', action='store_true', default=False,
        help='Produce coloured output.')

    args = parser.parse_args()
    colors = Colors(not args.no_color)

    # We will need to run objdump on the ELF file.
    kernel_elf_filename = args.kernel_elf_filename

    # This is the raw qemu.log file.
    coverage_filename = args.coverage_filename

    # Run objdump on the kernel binary.
    objdump_proc = Popen([get_tool('objdump'), '-d', '-j', '.text', kernel_elf_filename], stdout=PIPE)
    objdump_lines = objdump_proc.stdout.readlines()

    seL4_arm_vector_table_address = None

    # Now, parse the objdump file and retain the following data:

    # A map from address to line number within the objdump file.
    addr2lineno = {}

    # Can be seen as a map from line number in objdump file -> address.
    objdump_addreses = []

    # A map from function name to a set of all executable instructions within
    # it.
    function_instructions = {}

    current_function = None
    line_re = re.compile(r'^([0-9a-f]+):')
    ignore_re = re.compile(r'\.word|\.short|\.byte|undefined instruction')
    function_name_re = re.compile(r'^([0-9a-f]+) <([^>]+)>')
    for i, line in enumerate(objdump_lines):
        addr = None
        g = line_re.match(line)
        if g:
            g2 = ignore_re.search(line)
            if not g2:
                addr = int(g.group(1), 16)
                addr2lineno[addr] = i

        objdump_addreses.append(addr)

        if current_function is not None and addr is not None:
            function_instructions[current_function].add(addr)

        g = function_name_re.search(line)
        if g:
            current_function = g.group(2)
            function_instructions[current_function] = set()

            if current_function == 'arm_vector_table':
                seL4_arm_vector_table_address = int(g.group(1), 16)

    try:
        coverage_file = open(coverage_filename, 'r')
    except:
        print >>sys.stderr, 'Failed to open %s' % coverage_filename
        return -1

    # Record all executable instructions in the ELF file into a set.
    covered_instructions = set()
    trace_entry = re.compile(r'^Trace 0x[0-f]+ \[([0-f]+)\]')
    for line in coverage_file.readlines():
        entry = re.search(trace_entry, line)
        if not entry:
            continue
        addr = int(entry.group(1), 16)
        if addr in addr2lineno:
            covered_instructions.add(addr)

        # Sigh. And of course, here are some seL4-specific hacks. The vectors page
        # is not at the correct address in the binary. It is mapped at 0xffff0000
        # in memory, but starts at arm_vector_table in the binary. Account for that
        # here.
        if 0xffff0000 <= addr <= 0xffff1000 and seL4_arm_vector_table_address is not None:
            covered_instructions.add(addr - 0xffff0000 + seL4_arm_vector_table_address)
    coverage_file.close()

    # Print basic information.
    num_covered = len(covered_instructions)
    num_total = len(addr2lineno)
    print '%d/%d instructions covered (%.1f%%)' % (
         num_covered, num_total,
         100.0 * num_covered / num_total)

    if args.functions:
        # For each function, calculate how many instructions were covered.
        function_coverage = {}
        for f,instructions in function_instructions.iteritems():
            num_instructions = len(instructions)
            if num_instructions > 0:
                covered = len(instructions.intersection(covered_instructions))
                function_coverage[f] = (covered, num_instructions)

        # Sort by coverage and print.
        for f,x in sorted(function_coverage.items(), key=lambda (f,x): 1.0 * x[0] / x[1]):
            pct = 100.0 * x[0] / x[1]

            if pct == 0.0:
                colour = colors.LIGHT_RED
            elif pct == 100.0:
                colour = colors.DARK_GREEN
            else:
                colour = colors.DARK_YELLOW

            line = " %4d/%-4d %3.1f%% %s\n" % (x[0], x[1], pct, f)
            sys.stdout.write(colour + line + colors.NORMAL)

    if args.objdump:
        # Print a coloured objdump.
        for i, line in enumerate(objdump_lines):
            addr = objdump_addreses[i]
            covered = addr in covered_instructions
            valid = addr in addr2lineno
            if covered:
                colour = colors.DARK_GREEN
            elif valid:
                colour = colors.LIGHT_RED
            else:
                colour = colors.LIGHT_GREY

            sys.stdout.write(colour + line + colors.NORMAL)

    return 0

if __name__ == '__main__':
    sys.exit(main())
