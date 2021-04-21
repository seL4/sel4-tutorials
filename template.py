#!/usr/bin/env python3
#
# Copyright 2018, Data61, CSIRO (ABN 41 687 119 230)
#
# SPDX-License-Identifier: BSD-2-Clause
#
from __future__ import print_function
import os

from jinja2 import Environment, FileSystemLoader
import argparse
import sys
import sh
import tools
from tools import tutorialstate, context
from yaml import load, dump
try:
    from yaml import CLoader as Loader, CDumper as Dumper
except ImportError:
    from yaml import Loader, Dumper


def build_render_list(args):
    '''
    This function is pretty gross. It will likely be removed once the
    arguments that it understands are standardised as tutorials are ported
    to a consistent way. It figures out what initial file
    to render based on the tut_file argument.  The current behavior
    is to choose a .yaml or .md file if the exact filepath is passed in.
    If it is a yaml file, then the yaml file is loaded and its contents
    returned as data. If it is a md file, then this is added to the render
    list in data and returned.
    '''
    (dirname, leaf) = os.path.split(args.tut_file)
    data = {}

    yaml_file = ""
    md_file = ""
    if leaf.endswith(".yaml"):
        yaml_file = args.tut_file
    elif leaf.endswith(".md"):
        md_file = args.tut_file
    else:
        yaml_file = os.path.join(dirname, "%s.yaml" % leaf)
        md_file = os.path.join(dirname, "%s.md" % leaf)
    if os.path.isfile(yaml_file):
        with open(yaml_file, 'r') as stream:
            data = load(stream, Loader=Loader)
            # Save yaml file to input deps file
            if args.input_files:
                print(stream.name, file=args.input_files)
    elif os.path.isfile(md_file):
        data['render'] = [leaf if leaf.endswith(".md") else "%s.md" % leaf]
    else:
        print("Could not find a file to process", file=sys.stderr)
    return data


def render_file(args, env, state, file):
    '''
    Render a file. Any side effects that don't involve writing out to the filesystem
    should be captured by state. Any file that is touched should be added to the output_files
    file, and any files that are read should be added to the input_files file. This
    is for dependency tracking
    '''
    filename = os.path.join(args.out_dir, file)
    # Create required directories
    if not os.path.exists(os.path.dirname(filename)):
        os.makedirs(os.path.dirname(filename))

    with open(os.path.join(os.path.split(args.tut_file)[0], file), 'r') as in_stream, \
            open(filename, 'w') as out_stream:

        # Save dependencies to deps files
        if args.input_files and args.output_files:
            print(in_stream.name, file=args.input_files)
            print(out_stream.name, file=args.output_files)

        # process template file
        input = in_stream.read()
        template = env.from_string(input)

        out_stream.write(template.render(context.get_context(args, state)))


def save_script_imports(args):
    '''
    We save this file and every .py file in ./tools/ to the input_files
    dependency file.
    '''
    if args.input_files:
        print(os.path.realpath(__file__), file=args.input_files)
        tools_dir = os.path.join(os.path.dirname(__file__), "tools")
        for i in os.listdir(tools_dir):
            if i.endswith(".py"):
                print(os.path.realpath(os.path.join(tools_dir, i)), file=args.input_files)


def main():
    parser = argparse.ArgumentParser(description='Tutorial script template parser. Template is read from '
                                     'stdin and outout is placed in stdout')
    parser.add_argument('-s', '--solution', action='store_true', default=False)
    parser.add_argument('--docsite', action='store_true')
    parser.add_argument('--tut-file')
    parser.add_argument('--arch', default="x86_64")
    parser.add_argument('--rt', action='store_true')
    parser.add_argument('--task')
    parser.add_argument('--out-dir')
    parser.add_argument('--input-files', type=argparse.FileType('w'))
    parser.add_argument('--output-files', type=argparse.FileType('w'))
    args = parser.parse_args()

    # Save this script and its imports to input deps file
    save_script_imports(args)

    # Read list of files to generate into dict
    data = build_render_list(args)

    # Build our rendering environment.
    env = Environment(loader=FileSystemLoader(os.path.dirname(__file__)),
                      block_start_string='/*-',
                      block_end_string='-*/',
                      variable_start_string='/*?',
                      variable_end_string='?*/',
                      comment_start_string='/*#',
                      comment_end_string='#*/')
    env.filters.update(context.get_filters())

    # Init our tutorial state.
    state = tutorialstate.TuteState(args.task, args.solution, args.arch, args.rt)

    # Render all of the files.
    # We use a gross while True loop to allow state.additional_files to
    # be appended to as it is processed.
    for file in data['render']:
        render_file(args, env, state, file)

    while True:
        if not state.additional_files:
            break
        file = state.additional_files.pop(0)
        render_file(args, env, state, file)

    return 0


if __name__ == '__main__':
    sys.exit(main())
