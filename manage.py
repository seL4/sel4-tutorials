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

logger = logging.getLogger(__name__)

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
            os.path.join(self.tutorial_dir, 'solutions'))

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

    def create_build_symlinks(self):
        for source_path, link_path in self.list_build_symlinks():
            try:
                os.remove(link_path)
                logger.info("Removing old symlink: %s", link_path)
            except OSError:
                pass
            logger.info("Creating symlink: %s -> %s", link_path, source_path)
            os.symlink(source_path, link_path)

    def create_local_solutions(self):
        try:
            os.mkdir(self.local_solution_dir)
            logger.info("Creating solutions dir: %s" % self.local_solution_dir)

            for source, dest in self.list_solution_copies():
                logger.info("Instantiating solution: %s" % dest)
                self.template_ctx.copy_instantiating_templates(source, dest, solution=True)

            self.symlink_extra_apps(self.local_solution_dir)

        except OSError:
            logger.info("Solution dir already present: %s" % self.local_exercise_dir)


    def create_local_exercises(self):
        try:
            os.mkdir(self.local_exercise_dir)
            logger.info("Creating exercises dir: %s" % self.local_exercise_dir)

            for source, dest in self.list_exercise_copies():
                logger.info("Instantiating exercise: %s" % dest)
                self.template_ctx.copy_instantiating_templates(source, dest, solution=False)

            self.symlink_extra_apps(self.local_exercise_dir)

        except OSError:
            logger.info("Exercises dir already present: %s" % self.local_exercise_dir)

    def try_remove_app_symlink(self):
        try:
            os.remove(self.local_app_symlink)
            logger.info("Removing old symlink: %s" % self.local_app_symlink)
        except:
            pass

    def link_solutions(self):
        self.try_remove_app_symlink()
        os.symlink(self.local_solution_dir, self.local_app_symlink)
        logger.info("Creating app symlink: %s -> %s" % (self.local_app_symlink, self.local_exercise_dir))

    def link_exercises(self):
        self.try_remove_app_symlink()
        os.symlink(self.local_exercise_dir, self.local_app_symlink)
        logger.info("Creating app symlink: %s -> %s" % (self.local_app_symlink, self.local_exercise_dir))

    def setup(self):
        self.create_build_symlinks()
        self.create_local_solutions()
        self.create_local_exercises()

    def currently_is_exercise(self):
        return os.path.realpath(self.local_app_symlink) == self.local_exercise_dir

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

def make_parser():
    parser = argparse.ArgumentParser(description='Environment manager for seL4/CAmkES tutorials')

    parser.add_argument('commands', type=str, help='Valid commands: %s' % ", ".join(HANDLERS.keys()), nargs='*')

    return parser

def get_env():
    '''Determine environment by examining existing symlinks'''
    tutorial_type = common.get_tutorial_type()
    if tutorial_type is None:
        return None
    if tutorial_type == 'CAmkES':
        return CamkesEnvironment()
    elif tutorial_type == 'seL4':
        return Sel4Environment()

def handle_publish(git_uri, branch_name='master'):
    temp_dir = tempfile.mkdtemp(prefix='sel4-tutorials-')
    logger.info("Temporary directory created at: %s" % temp_dir)
    atexit.register(shutil.rmtree, temp_dir)

    git = sh.git.bake('-C', temp_dir, _out=sys.stdout)
    git.init()

    git.remote.add('origin', git_uri)
    git.fetch('origin')
    git.checkout('%s' % branch_name)

    logger.info("Removing existing repository files")
    for f in os.listdir(temp_dir):
        if f != '.git':
            path = os.path.join(temp_dir, f)
            sh.rm('-rf', path)

    # copy dirs/files that can be copied literally
    DIRS_TO_COPY = ['docs', 'build-config']
    FILES_TO_COPY = ['LICENSE_BSD2.txt', 'Prerequisites.md', 'README.md']

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

    template_ctx = TemplateCtx()
    template_dir = os.path.join(tutorial_dir, 'solutions')
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

def handle_env(env_name):
    try:
        old = get_env()
        new = ENVS[env_name]()
        new.setup()
        if old is None:
            new.link_exercises()
        else:
            if old.currently_is_exercise():
                new.link_exercises()
            else:
                new.link_solutions()
    except KeyError:
        logger.error("No such environment %s. Valid environments: %s" % (env_name, ", ".join(ENVS.keys())))

def handle_exercise():
    try:
        get_env().link_exercises()
    except AttributeError:
        logger.error("Environment not set up. `Run %s env {%s}`" % (__file__, "|".join(ENVS.keys())))

def handle_solution():
    try:
        get_env().link_solutions()
    except AttributeError:
        logger.error("Environment not set up. Run `%s env {%s}`" % (__file__, "|".join(ENVS.keys())))

def handle_run(arch, name, jobs="1"):
    run.main([name, '--arch', arch, '--jobs', jobs])

def handle_status():
    try:
        env = get_env()
        name = env.name
        mode = "exercise" if env.currently_is_exercise() else "solution"
        logger.info("%s %s" % (name, mode))
    except AttributeError:
        logger.error("Environment not set up. Run `%s env {%s}`" % (__file__, "|".join(ENVS.keys())))

HANDLERS = {
    "env": handle_env,
    "exercise": handle_exercise,
    "solution": handle_solution,
    "run": handle_run,
    "status": handle_status,
    "publish": handle_publish,
}

def run_command(command):
    try:
        # invoke the handler on the remaining arguments
        HANDLERS[command[0]](*command[1:])
    except KeyError:
        logger.error("Invalid command: %s. Valid commands: %s" % (command[0], ", ".join(HANDLERS.keys())))

def main(argv):
    common.setup_logger(__name__)

    args = make_parser().parse_args(argv)

    if len(args.commands) == 0:
        logger.error("Valid commands: %s" % ", ".join(HANDLERS.keys()))
        return -1

    run_command(args.commands)

if __name__ == '__main__':
    try:
        sys.exit(main(sys.argv[1:]))
    except KeyboardInterrupt:
        sys.exit(130)
