#
# Copyright 2017, Data61, CSIRO (ABN 41 687 119 230)
#
# SPDX-License-Identifier: BSD-2-Clause
#

from enum import Enum
import functools

from capdl import AllocatorState, ObjectAllocator, AddressSpaceAllocator, CSpaceAllocator, ObjectType, lookup_architecture


class TaskContentType(Enum):
    '''
    Task content type enum for describing when task content should be shown.
    BEFORE refers to content that should be shown before a task has been completed.
    COMPLETED refers to what the code should be when completed. Functions that
    print tasks out will either print BEFORE or COMPLETED, but not both.
    ALL is content that should be printed in both cases.
    '''
    BEFORE = 1
    COMPLETED = 2
    ALL = 3


@functools.total_ordering
class Task(object):
    '''
    Data object representing a task. A task contains content to be rendered during
    generation and completion text used to check if an assembled program is outputting
    as expected.
    subtasks are for describing different spacial locations where a task should output content
    tasks that output data to the same location should have the same subtask referring to that data.
    '''

    def __init__(self, name, index):
        self.index = index
        self.name = name
        self.subtask_content = {}
        self.content = {}
        self.completion = {}

    def __lt__(self, other):
        return self.index < other.index

    def __eq__(self, other):
        return self.index == other.index

    def set_content(self, content_type, content, subtask=None):
        '''
        Set content of a certain content_type for a task or subtask
        '''
        if subtask:
            if subtask not in self.subtask_content:
                self.subtask_content[subtask] = {}
            self.subtask_content[subtask][content_type] = content
        else:
            self.content[content_type] = content

    def get_content(self, content_type, subtask=None):
        '''
        Return the content for a particular subtask and content type
        '''
        if subtask:
            subtask_content = self.subtask_content.get(subtask)
            return subtask_content.get(content_type) if subtask_content else None
        return self.content.get(content_type)

    def set_completion(self, content_type, content):
        '''
        Set completion text for a task.
        It doesn't make sense for this to take a subtask
        '''
        self.completion[content_type] = content

    def get_completion(self, content_type):
        '''
        Get completion text for a task
        '''
        return self.completion.get(content_type)


class TuteState(object):
    '''
    Internal state of the tutorial that is being generated.
    It is assumed that the jinja template is evaluated from top to bottom
    like a program and that multiple templates are generated in a deterministic
    ordering. Because of this, code snippets are built up as the templates are
    processed and then used to generate tutorial files and other metadata.

    The state currently tracks the ordered list of tasks in the tutorial,
    the additional files to process, the current task that the tutorial is
    being rendered for and whether the rendering is in solution mode or not.
    Solution mode means that the solution for the current task will be generated
    instead of its starting state. Generally, the starting state of task 2 will be
    the same as the solution state of task 1, but this may not always be the case.
    '''

    def __init__(self, current_task, solution_mode, arch, rt):
        self.tasks = {}
        self.additional_files = []
        self.current_task = current_task
        self.solution = solution_mode
        self.stash = Stash(arch, rt)

    def declare_tasks(self, task_names):
        '''
        Declare the total tasks for the tutorial.
        Currently this can only be called once and declare all tutorials in one go.
        '''
        for (index, name) in enumerate(task_names):
            self.tasks[name] = Task(name, index)
        try:
            self.current_task = self.tasks[self.current_task]
        except KeyError:
            if self.solution:
                self.current_task = self.get_task_by_index(len(self.tasks) - 1)
            else:
                self.current_task = self.get_task_by_index(0)
        return

    def get_task(self, name):
        '''
        Get a Task based on its name
        '''
        return self.tasks[name]

    def get_current_task(self):
        '''
        Get a Task based on its name
        '''
        return self.current_task

    def is_current_task(self, task):
        '''
        Is a task the current task of the tutorial
        '''
        return task == self.current_task

    def get_task_by_index(self, id):
        '''
        Get a task by its index in the tutorial
        '''
        for (k, v) in self.tasks.items():
            if v.index == id:
                return v
        return None

    def print_task(self, task, subtask=None):
        '''
        Print a task or subtask content.
        If solution mode is active, then the COMPLETED content will be returned
        if the current_task is less than or equal to the provided task otherwise the BEFORE content
        will be returned. If solution mode is not active, then the COMPLETED content will be returned
        if the current_task is less than the provided task otherwise the BEFORE content will be returned.
        '''
        if self.solution:
            key = TaskContentType.COMPLETED if task <= self.current_task else TaskContentType.BEFORE
        else:
            key = TaskContentType.COMPLETED if task < self.current_task else TaskContentType.BEFORE
        content = task.get_content(key, subtask)
        if not content:
            content = task.get_content(TaskContentType.ALL, subtask)

        return content

    def print_completion(self, content_type):
        '''
        Return the completion text for a particular content type of the current task.
        If a completion text hasn't been defined for the BEFORE stage of a task, use the
        completion text from the COMPLETED part of the previous task
        '''
        def task_get_completion(task, key):
            ret = task.get_completion(key)
            if not ret:
                ret = task.get_completion(TaskContentType.ALL)
            return ret

        key = content_type
        completion = task_get_completion(self.current_task, key)
        if not completion and key == TaskContentType.BEFORE:
            assert self.current_task.index > 0
            completion = task_get_completion(self.get_task_by_index(
                self.current_task.index-1), TaskContentType.COMPLETED)
        # Reraise the error if we weren't requesting BEFORE. We require completion text defined for every stage
        if not completion:
            raise Exception("Failed to find completion for task {0}".format(self.current_task.name))
        return completion


class Stash(object):
    def __init__(self, arch, rt):
        self.rt = rt
        objects = ObjectAllocator()
        objects.spec.arch = arch
        self.allocator_state = AllocatorState(objects)
        self.current_cspace = None
        self.current_addr_space = None

        # The following variables are for tracking symbols to render before compilation
        self.elfs = {}
        self.current_cap_symbols = []
        self.cap_symbols = {}
        self.current_region_symbols = []
        self.region_symbols = {}

    def start_elf(self, name):
        cnode = self.allocator_state.obj_space.alloc(
            ObjectType.seL4_CapTableObject, "cnode_%s" % name)
        arch = self.allocator_state.obj_space.spec.arch.capdl_name()
        pd = self.allocator_state.obj_space.alloc(
            lookup_architecture(arch).vspace().object, "vspace_%s" % name)
        self.current_cspace = CSpaceAllocator(cnode)
        self.current_addr_space = AddressSpaceAllocator(None, pd)
        self.current_cap_symbols = []
        self.current_region_symbols = []

    def finish_elf(self, name, filename):
        self.allocator_state.addr_spaces[name] = self.current_addr_space
        self.allocator_state.cspaces[name] = self.current_cspace
        self.allocator_state.pds[name] = self.current_addr_space.vspace_root
        self.elfs[name] = {"filename": filename}
        self.cap_symbols[name] = self.current_cap_symbols
        self.region_symbols[name] = self.current_region_symbols
