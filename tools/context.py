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

import os, stat
from jinja2 import contextfilter, contextfunction

import macros
import tutorialstate
from tutorialstate import TaskContentType
from capdl import ObjectType, ObjectRights
from pickle import load, dumps


@contextfilter
def File(context, content, filename, **kwargs):
    '''
    Declare content to be written directly to a file
    '''
    args = context['args']
    if args.out_dir and not args.docsite:
        filename = os.path.join(args.out_dir, filename)
        if not os.path.exists(os.path.dirname(filename)):
            os.makedirs(os.path.dirname(filename))

        elf_file = open(filename, 'w')
        print(filename, file=args.output_files)
        elf_file.write(content)
        if "mode" in kwargs:
            if "executable" == kwargs["mode"]:
                st = os.stat(filename)
                os.chmod(filename, st.st_mode | stat.S_IEXEC)

    return content


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


@contextfilter
def ELF(context, content, name, passive=False):
    '''
    Declares a ELF object containing content with name.
    '''
    print("here")
    state = context['state']
    args = context['args']
    stash = state.stash

    print(content, name, context)
    if args.out_dir and not args.docsite:
        filename = os.path.join(args.out_dir, "%s.c" % name)
        if not os.path.exists(os.path.dirname(filename)):
            os.makedirs(os.path.dirname(filename))

        elf_file = open(filename, 'w')
        print(filename, file=args.output_files)

        elf_file.write(content)
        # elf_file.write("#line 1 \"thing\"\n" + content)

        stash.caps[name] = stash.unclaimed_caps
        stash.unclaimed_caps = []
        stash.elfs[name] = {"filename" :"%s.c" % name, "passive" : passive}
        stash.special_pages[name] = [("stack", 16*0x1000, 0x1000, 'guarded'),
        ("mainIpcBuffer", 0x1000, 0x1000, 'guarded'),
        ] + stash.unclaimed_special_pages
        stash.unclaimed_special_pages = []

    print("end")
    return content


@contextfunction
def ExternalFile(context, filename):
    '''
    Declare an additional file to be processed by the template renderer.
    '''
    state = context['state']
    state.additional_files.append(filename)
    return

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
    def normalise_task_name(task_name):
        subtask = None
        if isinstance(task_name,tuple):
            # Subtask
            subtask = task_name[1]
            task_name = task_name[0]
        return (task_name, subtask)


    if not isinstance(task_names, list):
        task_names = [task_names]
    state = context["state"]

    for (i, name) in enumerate(task_names):
        (name, subtask) = normalise_task_name(name)
        task = state.get_task(name)
        if task > state.get_current_task():
            if i is 0:
                # If we aren't up to any of the tasks yet we return nothing
                return ""
            # Use previous task
            (name, subtask) = normalise_task_name(task_names[i-1])
            task = state.get_task(name)
        elif task == state.get_current_task() or i is len(task_names) -1:
            # Use task as it is either the current task or there aren't more tasks to check
            pass
        else:
            # Check next task
            continue

        # We have a task, now we want to print it
        try:
            content = state.print_task(task, subtask)
            return content

        except KeyError:
            # If the start of task isn't defined then we print the previous task
            if i > 0:
                (name, subtask) = normalise_task_name(task_names[i-1])
                task = state.get_task(name)
                return state.print_task(task, subtask)
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
            content = ""
            try:
                print(task.name)
                content = state.print_task(task, subtask)
            except KeyError:
                if state.solution:
                    raise # In solution mode we require content.

            result.append(content)
    return '\n'.join(result)


@contextfunction
def declare_task_ordering(context, task_names):
    '''
    Declare the list of tasks that the tutorial contains.
    Their ordering in the array implies their ordering in the tutorial.
    '''
    state = context['state']
    state.declare_tasks(task_names)
    args = context['args']
    if args.out_dir and not args.docsite:
        filename = os.path.join(args.out_dir,".tasks")
        print(filename, file=args.output_files)
        if not os.path.exists(os.path.dirname(filename)):
            os.makedirs(os.path.dirname(filename))
        task_file = open(filename, 'w')
        for i in task_names:
            print(i,file=task_file)
    return ""


@contextfunction
def RecordObject(context, object, name, cap_symbol=None, **kwargs):
    print("Cap registered")
    state = context['state']
    stash = state.stash
    write = []
    if name in stash.objects:
        assert stash.objects[name][0] is object
        stash.objects[name][1].update(kwargs)
    else:
        if object is ObjectType.seL4_FrameObject:
            stash.unclaimed_special_pages.append((kwargs['symbol'], kwargs['size'], kwargs['alignment'], kwargs['section']))
            write.append("extern const char %s[%d];" % (kwargs['symbol'], kwargs['size']))
        elif object is not None:
            kwargs_new = {}
            kwargs_new.update(kwargs)
            stash.objects[name] = (object, kwargs_new)
    kwargs_new = {}
    kwargs_new.update(kwargs)
    stash.unclaimed_caps.append((cap_symbol, name, kwargs_new))
    if cap_symbol:
        write.append("extern seL4_CPtr %s;" % cap_symbol)
    return "\n".join(write)

@contextfunction
def capdl_elf_cspace(context, elf_name, cap_symbol):
    return RecordObject(context, None, "cnode_%s" % elf_name, cap_symbol=cap_symbol)

@contextfunction
def capdl_elf_vspace(context, elf_name, cap_symbol):
    return RecordObject(context, None, "vspace_%s" % elf_name, cap_symbol=cap_symbol)

@contextfunction
def capdl_elf_tcb(context, elf_name, cap_symbol):
    return RecordObject(context, None, "tcb_%s" % elf_name, cap_symbol=cap_symbol)

@contextfunction
def capdl_empty_slot(context, cap_symbol):
    return RecordObject(context, None, None, cap_symbol=cap_symbol)

@contextfunction
def capdl_declare_stack(context, size_bytes, stack_base_sym, stack_top_sym=None):
    declaration = RecordObject(context, ObjectType.seL4_FrameObject, ObjectType.seL4_FrameObject,
                   symbol=stack_base_sym, size=size_bytes, alignment=4096*2, section="guarded")
    stack_top = "" if stack_top_sym is None else "static const uintptr_t %s = (const uintptr_t)&%s + sizeof(%s);" % (stack_top_sym, stack_base_sym, stack_base_sym)
    return "\n".join([declaration.strip(), stack_top])

@contextfunction
def capdl_declare_frame(context, cap_symbol, symbol, size=4096):
    return RecordObject(context, ObjectType.seL4_FrameObject, ObjectType.seL4_FrameObject, cap_symbol=cap_symbol,
    symbol=symbol, size=size, alignment=size, section="guarded")

@contextfunction
def capdl_declare_ipc_buffer(context, cap_symbol, symbol):
    return capdl_declare_frame(context, cap_symbol, symbol)


@contextfunction
def write_manifest(context, file='manifest.py'):
    state = context['state']
    args = context['args']
    stash = state.stash
    if args.out_dir and not args.docsite:
        filename = os.path.join(args.out_dir, file)
        if not os.path.exists(os.path.dirname(filename)):
            os.makedirs(os.path.dirname(filename))

        file = open(filename, 'w')
        print(filename, file=args.output_files)


        manifest = """
import pickle

serialised = \"\"\"%s\"\"\"

# (OBJECTS, CSPACE_LAYOUT, SPECIAL_PAGES) = pickle.loads(serialised)
# print((OBJECTS, CSPACE_LAYOUT, SPECIAL_PAGES))
print(serialised)

        """
        file.write(manifest % dumps((stash.objects, stash.caps, stash.special_pages, stash.elfs)))
    return ""



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
    context = {
            "solution": args.solution,
            "args": args,
            "state": state,
            "include_task": include_task,
            "include_task_type_replace": include_task_type_replace,
            "include_task_type_append": include_task_type_append,
            "TaskContentType": TaskContentType,
            "ExternalFile": ExternalFile,
            "declare_task_ordering": declare_task_ordering,
            "macros": macros,
            "RecordObject": RecordObject,
            "write_manifest": write_manifest,
            "capdl_elf_cspace": capdl_elf_cspace,
            "capdl_elf_vspace": capdl_elf_vspace,
            "capdl_elf_tcb": capdl_elf_tcb,
            "capdl_empty_slot": capdl_empty_slot,
            "capdl_declare_stack": capdl_declare_stack,
            "capdl_declare_ipc_buffer": capdl_declare_ipc_buffer,
            "capdl_declare_frame": capdl_declare_frame,
            'tab':"\t",
    }

    # add all capDL object types
    context.update(ObjectType.__members__.items())

    # add all capDL object rights
    context.update(ObjectRights.__members__.items())

    return context


def get_filters():
    return {
        'File': File,
        'TaskContent': TaskContent,
        'TaskCompletion': TaskCompletion,
        'ExcludeDocs': ExcludeDocs,
        'ELF': ELF,
    }


