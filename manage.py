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

import jinja2
import argparse
import sys
import os
import re
import logging
import shutil
import atexit
import tempfile
import sh

import common
import run

logger = common.setup_logger(__name__)

class TemplateCtx(object):
    def __init__(self):
        self.template_dir = common.get_tutorial_dir()
        self.env = jinja2.Environment(
            loader=jinja2.FileSystemLoader(self.template_dir),
            block_start_string='/*-',
            block_end_string='-*/')

    def get_template(self, path):
        rel = os.path.relpath(path, self.template_dir)
        return self.env.get_template(rel)

    def render(self, path, **kwargs):
        return self.get_template(path).render(**kwargs)

    def copy_instantiating_templates(self, source_path, dest_path, **kwargs):
        for root, _dirs, files in os.walk(source_path):
            rel = os.path.relpath(root, source_path)
            cur_dir = os.path.realpath(os.path.join(dest_path, rel))
            os.mkdir(cur_dir)
            for f in files:
                inpath = os.path.join(root, f)
                outpath = os.path.join(cur_dir, f)
                # only instantiate templates in c and camkes files
                if f.endswith(".c") or f.endswith(".h") or f.endswith(".camkes"):
                    with open(outpath, 'w') as outfile:
                        outfile.write(self.render(inpath, **kwargs))
                else:
                    shutil.copyfile(inpath, outpath)

class BuildConfig(object):
    def __init__(self, name, prefix=None):

        if prefix is None:
            # assume name is a filename
            prefix, name = common.config_filename_to_parts(name)

        self.name = name
        self.prefix = prefix

    def filename(self):
        return common.config_filename_from_parts(self.prefix, self.name)

class Environment(object):
    def __init__(self, name):
        self.tutorial_dir = common.get_tutorial_dir()
        self.build_config_parent_dir = os.path.realpath(
            os.path.join(self.tutorial_dir, 'build-config'))
        self.project_root_dir = common.get_project_root()
        self.app_dir = os.path.realpath(
            os.path.join(self.tutorial_dir, 'templates'))

        self.tutorial_dir_rel = os.path.relpath(self.tutorial_dir, self.project_root_dir)
        self.build_config_parent_dir_rel = os.path.relpath(self.build_config_parent_dir,
                                                      self.project_root_dir)

        self.name = name

        if self.name == 'camkes':
            self.suffix = '-camkes'
        elif self.name == 'sel4':
            self.suffix = '-sel4'
        else:
            raise Exception("unknown environment name %s" % name)

        self.build_re = re.compile(r'(?P<name>.*)%s' % self.suffix)

        self.local_solution_dir = os.path.realpath(
            os.path.join(self.project_root_dir, 'apps%s-solutions' % self.suffix))

        self.local_exercise_dir = os.path.realpath(
            os.path.join(self.project_root_dir, 'apps%s-exercises' % self.suffix))

        self.local_template_dir = os.path.realpath(
            os.path.join(self.project_root_dir, 'apps%s-templates' % self.suffix))

        self.local_solution_dir_rel = os.path.relpath(
            self.app_dir, self.local_solution_dir)

        self.local_app_symlink = os.path.join(self.project_root_dir, 'apps')

        self.template_ctx = TemplateCtx()

    def symlink_extra_apps(self, dest):
        pass

    def build_config_dir(self):
        return os.path.join(self.build_config_parent_dir,
            'configs%s' % self.suffix)

    def list_build_config_filenames(self):
        return os.listdir(self.build_config_dir())

    def list_build_configs(self):
        return map(BuildConfig, self.list_build_config_filenames())

    def extra_app_names(self):
        return []

    def list_app_names(self):
        all_app_names = set(os.listdir(self.app_dir))
        return (set(c.name for c in self.list_build_configs()) | set(self.extra_app_names())) \
            & all_app_names

    def list_build_symlinks(self):
        for filename in os.listdir(self.build_config_parent_dir):
            m = self.build_re.match(filename)
            if m is not None:
                link_path = os.path.join(self.project_root_dir, m.group('name'))
                source_path = os.path.join(self.build_config_parent_dir_rel, filename)
                yield source_path, link_path

    def list_solution_copies(self):
        for name in self.list_app_names():
            source = os.path.join(self.app_dir, name)
            dest = os.path.join(self.local_solution_dir, name)
            yield source, dest

    def list_exercise_copies(self):
        for name in self.list_app_names():
            source = os.path.join(self.app_dir, name)
            dest = os.path.join(self.local_exercise_dir, name)
            yield source, dest

    def list_template_links(self):
        for name in self.list_app_names():
            source = os.path.join(self.app_dir, name)
            dest = os.path.join(self.local_template_dir, name)
            yield source, dest

    def create_build_symlinks(self):
        for source_path, link_path in self.list_build_symlinks():
            try:
                os.remove(link_path)
                logger.debug("Removing old symlink: %s", link_path)
            except OSError:
                pass
            logger.debug("Creating symlink: %s -> %s", link_path, source_path)
            os.symlink(source_path, link_path)

    def create_local_solutions(self):
        try:
            os.mkdir(self.local_solution_dir)
            logger.debug("Creating solutions dir: %s" % self.local_solution_dir)

            for source, dest in self.list_solution_copies():
                logger.info("Instantiating solution: %s" % dest)
                self.template_ctx.copy_instantiating_templates(source, dest, solution=True)

            self.symlink_extra_apps(self.local_solution_dir)

        except OSError:
            logger.debug("Solution dir already present: %s" % self.local_solution_dir)

    def reset(self):
        try:
            shutil.rmtree(self.local_solution_dir)
            shutil.rmtree(self.local_exercise_dir)
            shutil.rmtree(self.local_template_dir)
            os.remove(self.local_app_symlink)
        except OSError:
           pass

    def create_local_exercises(self):
        try:
            os.mkdir(self.local_exercise_dir)
            logger.debug("Creating exercises dir: %s" % self.local_exercise_dir)

            for source, dest in self.list_exercise_copies():
                logger.info("Instantiating exercise: %s" % dest)
                self.template_ctx.copy_instantiating_templates(source, dest, solution=False)

            self.symlink_extra_apps(self.local_exercise_dir)

        except OSError:
            logger.debug("Exercises dir already present: %s" % self.local_exercise_dir)

    def create_local_templates(self):
        try:
            os.mkdir(self.local_template_dir)
            logger.debug("Creating templates dir: %s" % self.local_template_dir)

            for source, dest in self.list_template_links():
                logger.info("Linking template: %s" % dest)
                os.symlink(source, dest)

            self.symlink_extra_apps(self.local_template_dir)

        except OSError:
            logger.debug("Templates dir already present: %s" % self.local_template_dir)

    def try_remove_app_symlink(self):
        try:
            os.remove(self.local_app_symlink)
            logger.debug("Removing old symlink: %s" % self.local_app_symlink)
        except:
            pass

    def link_solutions(self):
        self.try_remove_app_symlink()
        os.symlink(self.local_solution_dir, self.local_app_symlink)
        logger.debug("Creating app symlink: %s -> %s" % (self.local_app_symlink, self.local_exercise_dir))

    def link_exercises(self):
        self.try_remove_app_symlink()
        os.symlink(self.local_exercise_dir, self.local_app_symlink)
        logger.debug("Creating app symlink: %s -> %s" % (self.local_app_symlink, self.local_exercise_dir))

    def link_templates(self):
        '''Symlinks the tutorial templates directly into the apps directory
           to allow for development/debugging without needing to re-instantiate
           the templates. This works because template code is inside c comments,
           so the templates themselves are buildable/runnable, and behave like
           the tutorial solutions.'''
        self.try_remove_app_symlink()
        os.symlink(self.local_template_dir, self.local_app_symlink)
        logger.debug("Creating app symlink: %s -> %s" % (self.local_app_symlink, self.app_dir))

    def setup(self):
        self.create_build_symlinks()
        self.create_local_solutions()
        self.create_local_exercises()
        self.create_local_templates()

    def currently_is_exercise(self):
        return os.path.realpath(self.local_app_symlink) == self.local_exercise_dir

    def currently_is_solutions(self):
        return os.path.realpath(self.local_app_symlink) == self.local_solution_dir

    def currently_is_templates(self):
        return os.path.realpath(self.local_app_symlink) == self.local_template_dir

class Sel4Environment(Environment):
    def __init__(self):
        super(Sel4Environment, self).__init__('sel4')

    def extra_app_names(self):
        # these apps need to be copied, but don't have associated build configs
        # XXX there should be a way to generate this list dynamically
        return ['hello-4-app', 'hello-timer-client']

class CamkesEnvironment(Environment):
    def __init__(self):
        super(CamkesEnvironment, self).__init__('camkes')

    def symlink_extra_apps(self, path):
        capdl_loader_path = os.path.join(self.project_root_dir, 'projects', 'capdl', 'capdl-loader-app')
        rel = os.path.relpath(capdl_loader_path, path)
        os.symlink(rel, os.path.join(path, 'capdl-loader-experimental'))

def get_env():
    '''Determine environment by examining existing symlinks'''
    tutorial_type = common.get_tutorial_type()
    if tutorial_type is None:
        return None
    if tutorial_type == 'CAmkES':
        return CamkesEnvironment()
    elif tutorial_type == 'seL4':
        return Sel4Environment()

def handle_publish(args):
    git_uri = args.git
    branch_name = args.branch
    temp_dir = tempfile.mkdtemp(prefix='sel4-tutorials-')
    logger.info("Temporary directory created at: %s" % temp_dir)
    atexit.register(shutil.rmtree, temp_dir)

    git = sh.git.bake('-C', temp_dir, _out=sys.stdout)
    git.init()

    logger.info('git add origin %s', git_uri)
    git.remote.add('origin', git_uri)
    logger.info('git fetch origin')
    git.fetch('origin')

    logger.info('git checkout %s' % branch_name)
    if git.checkout('%s' % branch_name, _ok_code=[0,1]).exit_code == 1:
        logger.critical('Branch %s does not exist' % branch_name)
        return

    logger.debug("Removing existing repository files")
    for f in os.listdir(temp_dir):
        if f != '.git':
            path = os.path.join(temp_dir, f)
            sh.rm('-rf', path)

    # copy dirs/files that can be copied literally
    DIRS_TO_COPY = ['docs', 'build-config']
    FILES_TO_COPY = ['LICENSE_BSD2.txt', 'Prerequisites.md']

    tutorial_dir = common.get_tutorial_dir()

    for d in DIRS_TO_COPY:
        src = os.path.join(tutorial_dir, d)
        dst = os.path.join(temp_dir, d)
        logger.info("Copying directory: %s -> %s" % (src, dst))
        shutil.copytree(src, dst)

    for f in FILES_TO_COPY:
        src = os.path.join(tutorial_dir, f)
        dst = os.path.join(temp_dir, f)
        logger.info("Copying file: %s -> %s" % (src, dst))
        shutil.copyfile(src, dst)

    # copy the readme and rename
    src = os.path.join('README-tutorials.md', tutorial_dir)
    dst = os.path.join('README.md', temp_dir)
    logger.info("Copying file: %s -> %s" % (src, dst))
    shutil.copyfile(src, dst)

    template_ctx = TemplateCtx()
    template_dir = os.path.join(tutorial_dir, 'templates')
    solution_dir = os.path.join(temp_dir, 'solutions')
    exercise_dir = os.path.join(temp_dir, 'exercises')

    logger.info("Instantiating solution templates in: %s" % solution_dir)
    template_ctx.copy_instantiating_templates(template_dir, solution_dir, solution=True)

    logger.info("Instantiating exercise templates in: %s" % exercise_dir)
    template_ctx.copy_instantiating_templates(template_dir, exercise_dir, solution=False)

    logger.info("Starting interactive shell in git repository: %s" % temp_dir)
    logger.info("Press ctrl+d when done.")

    # give the user a shell in the new directory
    cwd = os.getcwd()
    os.chdir(temp_dir)
    sh.Command(os.getenv("SHELL", "/bin/sh"))(_fg=True)
    os.chdir(cwd)


ENVS = {
    'sel4': Sel4Environment,
    'camkes': CamkesEnvironment,
}

def env(new_env_name, is_template=False, is_solutions=False):
    new = ENVS[new_env_name]()
    new.setup()
    if is_template:
        new.link_templates()
    elif is_solutions:
        new.link_solutions()
    else:
        new.link_exercises()

def handle_env(args):
    old = get_env()
    if old is not None:
        env(args.ENV, old.currently_is_solutions(), old.currently_is_templates())
    else:
        env(args.ENV)

def add_sub_parser_env(subparsers):
    parser = subparsers.add_parser('env', help='Choose the tutorials environment')
    parser.add_argument('ENV', type=str, choices=list(ENVS))
    parser.set_defaults(func=handle_env)

def handle_exercise(args):
    try:
        get_env().link_exercises()
    except AttributeError:
        logger.error("Environment not set up. `Run %s env {%s}`" % (__file__, "|".join(ENVS.keys())))

def add_sub_parser_excercise(subparsers):
    parser = subparsers.add_parser('exercise', help='Switch the apps directory to show the tutorial exercises')
    parser.set_defaults(func=handle_exercise)

def handle_solution(args):
    try:
        get_env().link_solutions()
    except AttributeError:
        logger.error("Environment not set up. Run `%s env {%s}`" % (__file__, "|".join(ENVS.keys())))

def add_sub_parser_solution(subparsers):
    parser = subparsers.add_parser('solution', help='Switch the apps directory to show the tutorial solutions')
    parser.set_defaults(func=handle_solution)

def handle_template(args):
    try:
        get_env().link_templates()
    except AttributeError:
        logger.error("Environment not set up. Run `%s env {%s}`" % (__file__, "|".join(ENVS.keys())))

def add_sub_parser_template(subparsers):
    parser = subparsers.add_parser('template', help='Switch the apps directory to show the tutorial templates')
    parser.set_defaults(func=handle_template)

def handle_reset(args):
    # record these states before we reset as reset deletes the symlink used
    # to determine the current state of the enviroment
    template = get_env().currently_is_templates()
    solutions = get_env().currently_is_solutions()
    get_env().reset()
    env(get_env().name, is_solutions=solutions, is_template=template)

def add_sub_parser_reset(subparsers):
    parser = subparsers.add_parser('reset', help='Reset the environmenti by regenerating from templates')
    parser.set_defaults(func=handle_reset)

def handle_status(args):
    try:
        env = get_env()
        name = env.name
        mode = "exercise" if env.currently_is_exercise() else "solution"
        if env.currently_is_exercise():
            mode = "exercises"
        elif env.currently_is_solutions():
            mode = "solutions"
        elif env.currently_is_templates():
            mode = "templates"
        else:
            mode = "no mode set"

        logger.info("%s %s" % (name, mode))
    except AttributeError:
        logger.error("Environment not set up. Run `%s env {%s}`" % (__file__, "|".join(ENVS.keys())))

def add_sub_parser_status(subparsers):
    parser = subparsers.add_parser('status', help='Show the status of current environment')
    parser.set_defaults(func=handle_status)

def add_sub_parser_publish(subparsers):
    parser = subparsers.add_parser('publish', help='Publish the tutorials to a git repo')
    parser.add_argument('git', help='Git repo to publish to', type=str)
    parser.add_argument('branch', help='Branch to publish', type=str, default='master')
    parser.set_defaults(func=handle_publish)

def make_parser():
    parser = argparse.ArgumentParser(description='Environment manager for seL4/CAmkES tutorials')

    group = parser.add_mutually_exclusive_group()
    group.add_argument('-v','--verbose', action='store_true')
    group.add_argument('-q', '--quiet', action='store_true')
    subparsers = parser.add_subparsers(title='subcommands', description='Valid subcommands',
            help='For additional help, type \'manage SUBCOMMAND -h\'\n', dest='command')
    subparsers.required = True

    # add subparsers for each command
    add_sub_parser_env(subparsers)
    add_sub_parser_excercise(subparsers)
    add_sub_parser_solution(subparsers)
    add_sub_parser_template(subparsers)
    run.add_sub_parser_run(subparsers)
    add_sub_parser_status(subparsers)
    add_sub_parser_publish(subparsers)
    add_sub_parser_reset(subparsers)

    return parser


def main(argv):

    args = make_parser().parse_args(argv)

    if args.verbose:
        logger.setLevel(logging.DEBUG)
    elif args.quiet:
        logger.setLevel(logging.CRITICAL)
    else:
        logger.setLevel(logging.INFO)

    # invoke the handler on the remaining arguments
    args.func(args)


if __name__ == '__main__':
    try:
        sys.exit(main(sys.argv[1:]))
    except KeyboardInterrupt:
        sys.exit(130)
