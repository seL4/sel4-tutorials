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

from __future__ import print_function

import os
from jinja2 import contextfilter, contextfunction

import macros
import tutorialstate
from tutorialstate import TaskContentType

@contextfilter
def File(context, content, filename):
    '''
    Declare content to be written directly to a file
    '''
    args = context['args']
    if args.out_dir:
        filename = os.path.join(args.out_dir, filename)
        if not os.path.exists(os.path.dirname(filename)):
            os.makedirs(os.path.dirname(filename))

        elf_file = open(filename, 'w')
        print(filename, file=args.output_files)
        elf_file.write(content)

    return content

@contextfunction
def ExternalFile(context, filename):
    '''
    Declare an additional file to be processed by the template renderer.
    '''
    state = context['state']
    state.additional_files.append(filename)
    return


@contextfilter
def TaskContent(context, content, task_name, content_type=TaskContentType.COMPLETED, subtask=None, completion=None):
    '''
    Declare task content for a task. Optionally takes content type argument
    '''
    state = context["state"]
    task = state.get_task(task_name)
    task.set_content(content_type, content, subtask)
    if completion:
        task.set_completion(content_type, completion)
    return content

@contextfilter
def TaskCompletion(context, content, task_name, content_type):
    '''
    Declare completion text for a particular content_type
    '''
    state = context["state"]
    task = state.get_task(task_name)
    task.set_completion(content_type, content)
    return content



@contextfilter
def ExcludeDocs(context, content):
    '''
    Hides the contents from the documentation. Side effects from other functions
    and filters will still occur
    '''
    return ""

@contextfunction
def include_task(context, task_name, subtask=None):
    '''
    Prints a task out
    '''
    state = context["state"]
    task = state.get_task(task_name)
    return state.print_task(task, subtask)


@contextfunction
def include_task_type_replace(context, task_names):
    '''
    Takes a list of task names and displays only the one that is
    active in the tutorial
    '''
    if not isinstance(task_names, list):
        task_names = [task_names]
    state = context["state"]
    previous_task = None
    previous_subtask = None

    for (i, name) in enumerate(task_names):
        subtask = None
        if isinstance(name,tuple):
            # Subclass
            subtask = name[1]
            name = name[0]
        task = state.get_task(name)
        if not state.is_current_task(task):
            previous_task = task
            previous_subtask = subtask

        else:
            try:
                content = state.print_task(task, subtask)
                return content

            except KeyError:
                # If the start of task isn't defined then we print the previous task
                if i > 0:
                    return state.print_task(previous_task, previous_subtask)
                else:
                    return ""


    raise Exception("Could not find thing")

@contextfunction
def include_task_type_append(context, task_names):
    '''
    Takes a list of task_names and appends the task content based on the position in
    the tutorial.
    '''
    if not isinstance(task_names, list):
        task_names = [task_names]
    args = context['args']
    state = context["state"]

    result = []
    for i in task_names:
        subtask = None
        if isinstance(i,tuple):
            # Subclass
            subtask = i[1]
            i = i[0]
        task = state.get_task(i)
        if task <= state.current_task:
            result.append(state.print_task(task, subtask))
    return '\n'.join(result)


@contextfunction
def declare_task_ordering(context, task_names):
    '''
    Declare the list of tasks that the tutorial contains.
    Their ordering in the array implies their ordering in the tutorial.
    '''
    state = context['state']
    state.declare_tasks(task_names)
    return


'''
These are for potential extra template functions, that haven't been required
by templating requirements yet.
@contextfunction
def show_if_task(context, task_names):
    pass

@contextfunction
def show_before_task(context, task_names):
    pass


@contextfunction
def show_after_task(context, task_names):
    pass

@contextfunction
def hide_if_task(context, task_names):
    pass

@contextfunction
def hide_before_task(context, task_names):
    pass


@contextfunction
def hide_after_task(context, task_names):
    pass

'''




def get_context(args, state):
    return {
            "solution": args.solution,
            "args": args,
            "state": state,
            "include_task": include_task,
            "include_task_type_replace": include_task_type_replace,
            "include_task_type_append": include_task_type_append,
            "TaskContentType": TaskContentType,
            "ExternalFile": ExternalFile,
            "declare_task_ordering": declare_task_ordering,
            "macros": macros
    }


def get_filters():
    return {
        'File': File,
        'TaskContent': TaskContent,
        'TaskCompletion': TaskCompletion,
        'ExcludeDocs': ExcludeDocs,
    }


