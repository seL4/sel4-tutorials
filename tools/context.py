#
# Copyright 2017, Data61, CSIRO (ABN 41 687 119 230)
#
# SPDX-License-Identifier: BSD-2-Clause
#

from __future__ import print_function

import inspect
import os
import stat
from pickle import dumps
import pyaml
from jinja2 import pass_context

from . import macros
from capdl import ObjectType, ObjectRights, Cap, lookup_architecture
from .tutorialstate import TaskContentType


class TutorialFilters:
    """
    Class containing all tutorial filters. Add new static functions here to be included in the tutorial jinja2 context
    """

    @staticmethod
    @pass_context
    def File(context, content, filename, **kwargs):
        """
        Declare content to be written directly to a file
        """
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

    @staticmethod
    @pass_context
    def TaskContent(context, content, task_name, content_type, subtask=None, completion=None):
        """
        Declare task content for a task. Optionally takes content type argument
        """
        if not content_type or content_type not in TaskContentType:
            raise Exception("Invalid content type")

        state = context["state"]
        task = state.get_task(task_name)
        task.set_content(content_type, content, subtask)
        if completion:
            task.set_completion(content_type, completion)
        return content

    @staticmethod
    @pass_context
    def TaskCompletion(context, content, task_name, content_type):
        """
        Declare completion text for a particular content_type
        """
        if not content_type or content_type not in TaskContentType:
            raise Exception("Invalid content type")
        state = context["state"]
        task = state.get_task(task_name)
        task.set_completion(content_type, content)
        return content

    @staticmethod
    @pass_context
    def ExcludeDocs(context, content):
        """
        Hides the contents from the documentation. Side effects from other functions
        and filters will still occur
        """
        return ""

    @staticmethod
    @pass_context
    def ELF(context, content, name, passive=False):
        """
        Declares a ELF object containing content with name.
        """
        state = context['state']
        args = context['args']
        stash = state.stash

        if args.out_dir and not args.docsite:
            filename = os.path.join(args.out_dir, "%s.c" % name)
            if not os.path.exists(os.path.dirname(filename)):
                os.makedirs(os.path.dirname(filename))

            elf_file = open(filename, 'w')
            print(filename, file=args.output_files)

            elf_file.write(content)
            objects = stash.allocator_state.obj_space
            # The following allocates objects for the main thread, its IPC buffer and stack.
            stack_name = "stack"
            ipc_name = "mainIpcBuffer"
            number_stack_frames = 16
            frames = [objects.alloc(ObjectType.seL4_FrameObject, name='stack_%d_%s_obj' % (i, name), label=name, size=4096)
                      for i in range(number_stack_frames)]

            sizes = [4096] * (number_stack_frames)
            caps = [Cap(frame, read=True, write=True, grant=False) for frame in frames]
            stash.current_addr_space.add_symbol_with_caps(stack_name, sizes, caps)
            stash.current_region_symbols.append((stack_name, sum(sizes), 'size_12bit'))

            ipc_frame = objects.alloc(ObjectType.seL4_FrameObject,
                                      name='ipc_%s_obj' % (name), label=name, size=4096)
            caps = [Cap(ipc_frame, read=True, write=True, grant=False)]
            sizes = [4096]
            stash.current_addr_space.add_symbol_with_caps(ipc_name, sizes, caps)
            stash.current_region_symbols.append((ipc_name, sum(sizes), 'size_12bit'))

            tcb = objects.alloc(ObjectType.seL4_TCBObject, name='tcb_%s' % (name))
            tcb['ipc_buffer_slot'] = Cap(ipc_frame, read=True, write=True, grant=False)
            cap = Cap(stash.current_cspace.cnode)
            tcb['cspace'] = cap
            stash.current_cspace.cnode.update_guard_size_caps.append(cap)
            tcb['vspace'] = Cap(stash.current_addr_space.vspace_root)
            tcb.sp = "get_vaddr(\'%s\') + %d" % (stack_name, sum(sizes))
            tcb.addr = "get_vaddr(\'%s\')" % (ipc_name)
            tcb.ip = "get_vaddr(\'%s\')" % ("_start")
            # This initialises the main thread's stack so that a normal _start routine provided by libmuslc can be used
            # The capdl loader app takes the first 4 arguments of .init and sets registers to them,
            # argc = 2,
            # argv[0] = get_vaddr("progname") which is a string of the program name,
            # argv[1] = 1 This could be changed to anything,
            # 0, 0, null terminates the argument vector and null terminates the empty environment string vector
            # 32, is an aux vector key of AT_SYSINFO
            # get_vaddr(\"sel4_vsyscall\") is the address of the SYSINFO table
            # 0, 0, null terminates the aux vectors.
            tcb.init = "[0,0,0,0,2,get_vaddr(\"progname\"),1,0,0,32,get_vaddr(\"sel4_vsyscall\"),0,0]"
            if not passive and stash.rt:
                sc = objects.alloc(ObjectType.seL4_SchedContextObject,
                                   name='sc_%s_obj' % (name), label=name)
                sc.size_bits = 8
                tcb['sc_slot'] = Cap(sc)
            stash.current_cspace.alloc(tcb)
            stash.finish_elf(name, "%s.c" % name)

        print("end")
        return content


class TutorialFunctions:
    """Class containing all tutorial functions. Add new static functions here to be included in the tutorial jinja2
    context """

    @staticmethod
    @pass_context
    def ExternalFile(context, filename):
        """
        Declare an additional file to be processed by the template renderer.
        """
        state = context['state']
        state.additional_files.append(filename)
        return

    @staticmethod
    @pass_context
    def include_task(context, task_name, subtask=None):
        """
        Prints a task out
        """
        state = context["state"]
        task = state.get_task(task_name)
        content = state.print_task(task, subtask)
        if not task:
            raise Exception("No content found for {0} {1}".format(task_name, str(subtask or '')))

    @staticmethod
    @pass_context
    def include_task_type_replace(context, task_names):
        """
        Takes a list of task names and displays only the one that is
        active in the tutorial
        """
        def normalise_task_name(task_name):
            subtask = None
            if isinstance(task_name, tuple):
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
                if i == 0:
                    # If we aren't up to any of the tasks yet we return nothing
                    return ""
                # Use previous task
                (name, subtask) = normalise_task_name(task_names[i-1])
                task = state.get_task(name)
            elif task == state.get_current_task() or i is len(task_names) - 1:
                # Use task as it is either the current task or there aren't more tasks to check
                pass
            else:
                # Check next task
                continue

            # We have a task, now we want to print it
            content = state.print_task(task, subtask)
            if not content:
                # If the start of task isn't defined then we print the previous task
                if i > 0:
                    (name, subtask) = normalise_task_name(task_names[i-1])
                    task = state.get_task(name)
                    content = state.print_task(task, subtask)
            return str(content or '')

        raise Exception("Could not find thing")

    @staticmethod
    @pass_context
    def include_task_type_append(context, task_names):
        """
        Takes a list of task_names and appends the task content based on the position in
        the tutorial.
        """
        if not isinstance(task_names, list):
            task_names = [task_names]
        args = context['args']
        state = context["state"]

        result = []
        for i in task_names:
            subtask = None
            if isinstance(i, tuple):
                # Subclass
                subtask = i[1]
                i = i[0]
            task = state.get_task(i)
            if task <= state.current_task:
                print(task.name)
                content = state.print_task(task, subtask)
                if not content:
                    if state.solution:
                        raise Exception("No content found for {0} {1}".format(
                            task, str(subtask or '')))
                result.append(str(content or ''))
        return '\n'.join(result)

    @staticmethod
    @pass_context
    def declare_task_ordering(context, task_names):
        """
        Declare the list of tasks that the tutorial contains.
        Their ordering in the array implies their ordering in the tutorial.
        """
        state = context['state']
        state.declare_tasks(task_names)
        args = context['args']
        if args.out_dir and not args.docsite:
            filename = os.path.join(args.out_dir, ".tasks")
            print(filename, file=args.output_files)
            if not os.path.exists(os.path.dirname(filename)):
                os.makedirs(os.path.dirname(filename))
            task_file = open(filename, 'w')
            for i in task_names:
                print(i, file=task_file)
        return ""

    @staticmethod
    @pass_context
    def capdl_alloc_obj(context, obj_type, obj_name, **kwargs):
        state = context['state']
        stash = state.stash
        obj = None
        if obj_type and obj_name:
            obj = stash.allocator_state.obj_space.alloc(obj_type, obj_name, **kwargs)
        return obj

    @staticmethod
    @pass_context
    def capdl_alloc_cap(context, obj_type, obj_name, symbol, **kwargs):
        """
        Alloc a cap and emit a symbol for it.
        """
        state = context['state']
        stash = state.stash
        obj = TutorialFunctions.capdl_alloc_obj(context, obj_type, obj_name)
        slot = stash.current_cspace.alloc(obj, **kwargs)
        stash.current_cap_symbols.append((symbol, slot))
        return "extern seL4_CPtr %s;" % symbol

    @staticmethod
    @pass_context
    def capdl_elf_cspace(context, elf_name, cap_symbol):
        return TutorialFunctions.capdl_alloc_cap(context, ObjectType.seL4_CapTableObject, "cnode_%s" % elf_name, cap_symbol)

    @staticmethod
    @pass_context
    def capdl_elf_vspace(context, elf_name, cap_symbol):
        pd_type = lookup_architecture("x86_64").vspace().object
        return TutorialFunctions.capdl_alloc_cap(context, pd_type, "vspace_%s" % elf_name, cap_symbol)

    @staticmethod
    @pass_context
    def capdl_elf_tcb(context, elf_name, cap_symbol):
        return TutorialFunctions.capdl_alloc_cap(context, ObjectType.seL4_TCBObject, "tcb_%s" % elf_name, cap_symbol)

    @staticmethod
    @pass_context
    def capdl_elf_sc(context, elf_name, cap_symbol):
        return TutorialFunctions.capdl_alloc_cap(context, ObjectType.seL4_SchedContextObject, "sc_%s" % elf_name, cap_symbol)

    @staticmethod
    @pass_context
    def capdl_sched_control(context, cap_symbol):
        return TutorialFunctions.capdl_alloc_cap(context, ObjectType.seL4_SchedControl, "sched_control", cap_symbol)

    @staticmethod
    @pass_context
    def capdl_irq_control(context, cap_symbol):
        return TutorialFunctions.capdl_alloc_cap(context, ObjectType.seL4_IRQControl, "irq_control", cap_symbol)

    @staticmethod
    @pass_context
    def capdl_empty_slot(context, cap_symbol):
        return TutorialFunctions.capdl_alloc_cap(context, None, None, cap_symbol)

    @staticmethod
    @pass_context
    def capdl_declare_stack(context, size_bytes, stack_base_sym, stack_top_sym=None):
        state = context['state']
        stash = state.stash
        stash.current_region_symbols.append((stack_base_sym, size_bytes, "size_12bit"))
        return "\n".join([
            "extern const char %s[%d];" % (stack_base_sym, size_bytes),
            "" if stack_top_sym is None else "static const uintptr_t %s = (const uintptr_t)&%s + sizeof(%s);" % (
                stack_top_sym, stack_base_sym, stack_base_sym)
        ])

    @staticmethod
    @pass_context
    def capdl_declare_frame(context, cap_symbol, symbol, size=4096):
        state = context['state']
        stash = state.stash

        obj = TutorialFunctions.capdl_alloc_obj(
            context, ObjectType.seL4_FrameObject, cap_symbol, size=size)
        cap_symbol = TutorialFunctions.capdl_alloc_cap(
            context, ObjectType.seL4_FrameObject, cap_symbol, cap_symbol, read=True, write=True, grant=True)
        stash.current_addr_space.add_symbol_with_caps(
            symbol, [size], [Cap(obj, read=True, write=True, grant=True)])
        stash.current_region_symbols.append((symbol, size, "size_12bit"))
        return "\n".join([
            cap_symbol,
            "extern const char %s[%d];" % (symbol, size),
        ])

    @staticmethod
    @pass_context
    def capdl_declare_ipc_buffer(context, cap_symbol, symbol):
        return TutorialFunctions.capdl_declare_frame(context, cap_symbol, symbol)

    @staticmethod
    @pass_context
    def write_manifest(context, manifest='manifest.yaml', allocator="allocators.pickle"):
        state = context['state']
        args = context['args']
        stash = state.stash
        if args.out_dir and not args.docsite:
            manifest = os.path.join(args.out_dir, manifest)
            allocator = os.path.join(args.out_dir, allocator)
            if not os.path.exists(os.path.dirname(manifest)):
                os.makedirs(os.path.dirname(manifest))

            manifest_file = open(manifest, 'w')
            allocator_file = open(allocator, 'wb')
            print(manifest, file=args.output_files)
            print(allocator, file=args.output_files)

            data = {"cap_symbols": stash.cap_symbols, "region_symbols": stash.region_symbols}

            manifest_file.write(pyaml.dump(data))
            allocator_file.write(dumps(stash.allocator_state))
        return ""


'''
These are for potential extra template functions, that haven't been required
by templating requirements yet.
@pass_context
def show_if_task(context, task_names):
    pass

@pass_context
def show_before_task(context, task_names):
    pass


@pass_context
def show_after_task(context, task_names):
    pass

@pass_context
def hide_if_task(context, task_names):
    pass

@pass_context
def hide_before_task(context, task_names):
    pass


@pass_context
def hide_after_task(context, task_names):
    pass

'''


def get_context(args, state):
    context = {
        "solution": args.solution,
        "args": args,
        "state": state,
        "TaskContentType": TaskContentType,
        "macros": macros,
        'tab': "\t",
    }

    # add all capDL object types
    context.update(ObjectType.__members__.items())

    # add all capDL object rights
    context.update(ObjectRights.__members__.items())

    # add all the static functions in TutorialFunctions. To
    # add a new function to the context, add it as a static method to
    # the TutorialFunctions class above.
    context.update(inspect.getmembers(TutorialFunctions, predicate=inspect.isfunction))

    return context


def get_filters():
    # add all static functions from the TutorialFilters class. To add a new
    # filter, add a new static function to the TutorialFilters class above.
    return inspect.getmembers(TutorialFilters, predicate=inspect.isfunction)
