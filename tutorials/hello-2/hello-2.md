---
toc: true
---
/*? declare_task_ordering(
['task-1', 
'task-2',
'task-3',
'task-4',
'task-5',
'task-6',
'task-7',
'task-8',
'task-9',
'task-10',
'task-11',
'task-12',
'task-13',
'task-14',
'task-15',
]) ?*/

# seL4 Tutorial 2
The second tutorial is useful in that
it addresses conceptual problems for two different types of developers:

- Experienced kernel developers whose minds are pre-programmed to
      think in terms of "One address space equals one process", and
      begins to introduce the seL4 CSpace vs VSpace model.
- New kernel developers, for whom the tutorial will prompt them on
      what to read about.

Don't gloss over the globals declared before `main()` -- they're declared
for your benefit so you can grasp some of the basic data structures.
Uncomment them one by one as needed when going through the tasks.

## Learning outcomes:


- Understand the kernel's startup procedure.
- Understand that the kernel centers around certain objects and
        capabilities to those objects.
- Understand that libraries exist to automate the very
        fine-grained nature of the seL4 API, and get a rough idea of
        some of those libraries.
- Learn how the kernel hands over control to userspace.
- Get a feel for how the seL4 API enables developers to manipulate
        the objects referred to by the capabilities they encounter.
- Understand the how to spawn new threads in seL4, and the basic
        idea that a thread has a TCB, VSpace and CSpace, and that you
        must fill these out.

## Walkthrough
```
# create a build directory
mkdir build_hello_2
cd build_hello_2
# initialise your build directory
../init --plat pc99 --tut hello-2
```

Look for `TASK` in the `hello-2/src` directory for each task.

### TASK 1


After bootstrap, the kernel hands over control to to an init thread.
This thread receives a structure from the kernel that describes all the
resources available on the machine. This structure is called the
BootInfo structure. It includes information on all IRQs, memory, and
IO-Ports (x86). This structure also tells the init thread where certain
important capability references are. This step is teaching you how to
obtain that structure.

`seL4_BootInfo* platsupport_get_bootinfo(void)` is a function that returns the BootInfo structure.
It also sets up the IPC buffer so that it can perform some syscalls such as `seL4_DebugNameThread` used by `name_thread`.

```c
int main(void) {

/*- filter TaskContent("task-1", TaskContentType.ALL, completion="main: hello world") -*/
    /* TASK 1: get boot info */
    /* hint: platsupport_get_bootinfo()
     * seL4_BootInfo* platsupport_get_bootinfo(void);
     * @return Pointer to the bootinfo, NULL on failure
     * Links to source: https://docs.sel4.systems/Tutorials/seL4_Tutorial_2#task-1:
     */
    info = platsupport_get_bootinfo();
    ZF_LOGF_IF(info == NULL, "Failed to get bootinfo.");
/*- endfilter -*/
}
```

To build and run:
```
# build it
ninja
# run it in qemu
./simulate
```

until Task 4, where you set up memory management.

<https://github.com/seL4/seL4/blob/master/libsel4/include/sel4/bootinfo_types.h>
<https://github.com/seL4/seL4_libs/blob/master/libsel4platsupport/src/bootinfo.c>
### TASK 2 init simple

/* TASK 2:  */
/* hint: simple_default_init_bootinfo()
 * void simple_default_init_bootinfo(simple_t *simple, seL4_BootInfo *bi);
 * @param simple Structure for the simple interface object. This gets initialised.
 * @param bi Pointer to the bootinfo describing what resources are available
 * Links to source: https://docs.sel4.systems/Tutorials/seL4_Tutorial_2#task-2:
 */


```c
/*- filter TaskContent("task-2", TaskContentType.COMPLETED, completion="main: hello world") -*/
    simple_default_init_bootinfo(&simple, info);
/*- endfilter -*/
```


The "Simple" library is one of those you were introduced to in the
slides: you need to initialize it with some default state before using
it.

<https://github.com/seL4/seL4_libs/blob/master/libsel4simple-default/include/simple-default/simple-default.h>

### TASK 3

```c
/*- filter TaskContent("task-3", TaskContentType.COMPLETED, completion="main: hello world") -*/

    /* TASK 3: print out bootinfo and other info about simple */
    /* hint: simple_print()
     * void simple_print(simple_t *simple);
     * @param simple Pointer to simple interface.
     * Links to source: https://docs.sel4.systems/Tutorials/seL4_Tutorial_2#task-3:
     */
    simple_print(&simple);

/*- endfilter -*/
```

Just a simple debugging print-out function. Allows you to examine the
layout of the BootInfo.

<https://github.com/seL4/seL4_libs/blob/master/libsel4simple/include/simple/simple.h>

### TASK 4

```c
/*- filter TaskContent("task-4", TaskContentType.COMPLETED, completion="main: hello world") -*/

    /* TASK 4: create an allocator */
    /* hint: bootstrap_use_current_simple()
     * allocman_t *bootstrap_use_current_simple(simple_t *simple, uint32_t pool_size, char *pool);
     * @param simple Pointer to simple interface.
     * @param pool_size Size of the initial memory pool.
     * @param pool Initial memory pool.
     * @return returns NULL on error
     * Links to source: https://docs.sel4.systems/Tutorials/seL4_Tutorial_2#task-4:
     */
    allocman = bootstrap_use_current_simple(&simple, ALLOCATOR_STATIC_POOL_SIZE, allocator_mem_pool);
    ZF_LOGF_IF(allocman == NULL, "Failed to initialize alloc manager.\n"
               "\tMemory pool sufficiently sized?\n"
               "\tMemory pool pointer valid?\n");

/*- endfilter -*/
```

In seL4, memory management is delegated in large part to userspace, and
each process manages its own page faults with a custom pager. Without
the use of the `allocman` library and the `VKA` library, you would have to
manually allocate a frame, then map the frame into a page-table, before
you could use new memory in your address space. In this tutorial you
don't go through that procedure, but you'll encounter it later. For now,
use the allocman and VKA allocation system. The allocman library
requires some initial memory to bootstrap its metadata. Complete this
step.

<https://github.com/seL4/seL4_libs/blob/master/libsel4allocman/include/allocman/bootstrap.h>

### TASK 5

```c
/*- filter TaskContent("task-5", TaskContentType.COMPLETED, completion="main: hello world") -*/

    /* TASK 5: create a vka (interface for interacting with the underlying allocator) */
    /* hint: allocman_make_vka()
     * void allocman_make_vka(vka_t *vka, allocman_t *alloc);
     * @param vka Structure for the vka interface object.  This gets initialised.
     * @param alloc allocator to be used with this vka
     * Links to source: https://docs.sel4.systems/Tutorials/seL4_Tutorial_2#task-5:
     */
    allocman_make_vka(&vka, allocman);
/*- endfilter -*/
```

`libsel4vka` is an seL4 type-aware object allocator that will allocate new
kernel objects for you. The term "allocate new kernel objects" in seL4
is a more detailed process of "retyping" previously un-typed memory.
seL4 considers all memory that hasn't been explicitly earmarked for a
purpose to be "untyped", and in order to repurpose any memory into a
useful object, you must give it an seL4-specific type. This is retyping,
and the VKA library simplifies this for you, among other things.

<https://github.com/seL4/seL4_libs/blob/master/libsel4allocman/include/allocman/vka.h>

### TASK 6

```c
/*- filter TaskContent("task-6", TaskContentType.COMPLETED, completion="main: hello world") -*/

    /* TASK 6: get our cspace root cnode */
    /* hint: simple_get_cnode()
     * seL4_CPtr simple_get_cnode(simple_t *simple);
     * @param simple Pointer to simple interface.
     * @return The cnode backing the simple interface. no failure.
     * Links to source: https://docs.sel4.systems/Tutorials/seL4_Tutorial_2#task-6:
     */
    seL4_CPtr cspace_cap;
    cspace_cap = simple_get_cnode(&simple);
/*- endfilter -*/
```

This is where the differences between seL4 and contemporary kernels
begin to start playing out. Every kernel-object that you "retype" will
be handed to you using a capability reference. The seL4 kernel keeps
multiple trees of these capabilities. Each separate tree of capabilities
is called a "CSpace". Each thread can have its own CSpace, or a CSpace
can be shared among multiple threads. The delineations between
"Processes" aren't well-defined, since seL4 doesn't have any real
concept of "processes". It deals with threads. Sharing and isolation is
based on CSpaces (shared vs not-shared) and VSpaces (shared vs
not-shared). The "process" idea goes as far as perhaps the fact that at
the hardware level, threads sharing the same VSpace are in a traditional
sense, siblings, but logically in seL4, there is no concept of
"Processes" really.

So you're being made to grab a reference to your thread's CSpace's root
"CNode". A CNode is one of the many blocks of capabilities that make up
a CSpace.

<https://github.com/seL4/seL4_libs/blob/master/libsel4simple/include/simple/simple.h>

### TASK 7

```c
/*- filter TaskContent("task-7", TaskContentType.COMPLETED, completion="main: hello world") -*/

    /* TASK 7: get our vspace root page diretory */
    /* hint: simple_get_pd()
     * seL4_CPtr simple_get_pd(simple_t *simple);
     * @param simple Pointer to simple interface.
     * @return The vspace (PD) backing the simple interface. no failure.
     * Links to source: https://docs.sel4.systems/Tutorials/seL4_Tutorial_2#task-7:
     */
    seL4_CPtr pd_cap;
    pd_cap = simple_get_pd(&simple);

/*- endfilter -*/
```

Just as in the previous step, you were made to grab a reference to the
root of your thread's CSpace, now you're being made to grab a reference
to the root of your thread's VSpace.

<https://github.com/seL4/seL4_libs/blob/master/libsel4simple/include/simple/simple.h>

### TASK 8

```c
/*- filter TaskContent("task-8", TaskContentType.COMPLETED, completion="main: hello world") -*/

    /* TASK 8: create a new TCB */
    /* hint: vka_alloc_tcb()
     * int vka_alloc_tcb(vka_t *vka, vka_object_t *result);
     * @param vka Pointer to vka interface.
     * @param result Structure for the TCB object.  This gets initialised.
     * @return 0 on success
     * https://docs.sel4.systems/Tutorials/seL4_Tutorial_2#task-8:
     */
    vka_object_t tcb_object = {0};
    error = vka_alloc_tcb(&vka, &tcb_object);
    ZF_LOGF_IFERR(error, "Failed to allocate new TCB.\n"
                  "\tVKA given sufficient bootstrap memory?");

/*- endfilter -*/
```

In order to manage the threads that are created in seL4, the seL4 kernel
keeps track of TCB (Thread Control Block) objects. Each of these
represents a schedulable executable resource. Unlike other contemporary
kernels, seL4 **doesn't** allocate a stack, virtual-address space
(VSpace) and other metadata on your behalf. This step creates a TCB,
which is a very bare-bones, primitive resource, which requires you to
still manually fill it out.

<https://github.com/seL4/seL4_libs/blob/master/libsel4vka/include/vka/object.h>

### TASK 9

```c
/*- filter TaskContent("task-9", TaskContentType.COMPLETED, completion="main: hello world") -*/
   /* TASK 9: initialise the new TCB */
    /* hint 1: seL4_TCB_Configure()
     * int seL4_TCB_Configure(seL4_TCB _service, seL4_Word fault_ep, seL4_CNode cspace_root, seL4_Word cspace_root_data, seL4_CNode vspace_root, seL4_Word vspace_root_data, seL4_Word buffer, seL4_CPtr bufferFrame)
     * @param service Capability to the TCB which is being operated on.
     * @param fault_ep Endpoint which receives IPCs when this thread faults (must be in TCB's cspace).
     * @param cspace_root The new CSpace root.
     * @param cspace_root_data Optionally set the guard and guard size of the new root CNode. If set to zero, this parameter has no effect.
     * @param vspace_root The new VSpace root.
     * @param vspace_root_data Has no effect on IA-32 or ARM processors.
     * @param buffer Address of the thread's IPC buffer. Must be 512-byte aligned. The IPC buffer may not cross a page boundary.
     * @param bufferFrame Capability to a page containing the thread?s IPC buffer.
     * @return 0 on success.
     * Note: this function is generated during build.  It is generated from the following definition:
     * Links to source: https://docs.sel4.systems/Tutorials/seL4_Tutorial_2#task-9:
     * You can find out more about it in the API manual: http://sel4.systems/Info/Docs/seL4-manual-latest.pdf
     *
     * hint 2: use seL4_CapNull for the fault endpoint
     * hint 3: use seL4_NilData for cspace and vspace data
     * hint 4: we don't need an IPC buffer frame or address yet
     */
    error = seL4_TCB_Configure(tcb_object.cptr, seL4_CapNull,  cspace_cap, seL4_NilData, pd_cap, seL4_NilData, 0, 0);
    ZF_LOGF_IFERR(error, "Failed to configure the new TCB object.\n"
                  "\tWe're running the new thread with the root thread's CSpace.\n"
                  "\tWe're running the new thread in the root thread's VSpace.\n"
                  "\tWe will not be executing any IPC in this app.\n");

    /* Set the priority of the new thread to be equal to our priority. This ensures it will run
     * in round robin with us. By default it has priority of 0 and so would never run unless we block */
    error = seL4_TCB_SetPriority(tcb_object.cptr, simple_get_tcb(&simple), 255);
    ZF_LOGF_IFERR(error, "Failed to set the priority for the new TCB object.\n");
/*- endfilter -*/
```

You must create a new VSpace for your new thread if you need it to
execute in its own isolated address space, and tell the kernel which
VSpace you plan for the new thread to execute in. This opens up the
option for threads to share VSpaces. In similar fashion, you must also
tell the kernel which CSpace your new thread will use -- whether it will
share a currently existing one, or whether you've created a new one for
it. That's what you're doing now.

In this particular example, you're allowing the new thread to share your
main thread's CSpace and VSpace.

In addition, a thread needs to have a priority set on it in order for it to run.
`seL4_TCB_SetPriority(tcb_object.cptr, seL4_CapInitThreadTCB, seL4_MaxPrio);`
will give your new thread the same priority as the current thread, allowing it
to be run the next time the seL4 scheduler is invoked.  The seL4 scheduler is invoked
everytime there is a kernel timer tick.

<https://github.com/seL4/seL4/blob/master/libsel4/include/interfaces/sel4.xml>

### TASK 10

```c
/*- filter TaskContent("task-10", TaskContentType.COMPLETED, completion="main: hello world") -*/
    /* TASK 10: give the new thread a name */
    /* hint: we've done thread naming before */
    name_thread(tcb_object.cptr, "hello-2: thread_2");

/*- endfilter -*/
```

This is a convenience function -- sets a name string for the TCB object.

### TASK 11

```c
/*- filter TaskContent("task-11", TaskContentType.COMPLETED, completion="main: hello world") -*/

    /*
     * set start up registers for the new thread:
     */

    UNUSED seL4_UserContext regs = {0};

    /* TASK 11: set instruction pointer where the thread shoud start running */
    /* hint 1: sel4utils_set_instruction_pointer()
     * void sel4utils_set_instruction_pointer(seL4_UserContext *regs, seL4_Word value);
     * @param regs Data structure in which to set the instruction pointer value
     * @param value New instruction pointer value
     * Links to source: https://docs.sel4.systems/Tutorials/seL4_Tutorial_2#task-11:
     *
     * hint 2: we want the new thread to run the function "thread_2"
     */
    sel4utils_set_instruction_pointer(&regs, (seL4_Word)thread_2);

/*- endfilter -*/
```

Pay attention to the line that precedes this particular task -- the line
that zeroes out a new "seL4_UserContext" object. As we previously
explained, seL4 requires you to fill out the Thread Control Block
manually. That includes the new thread's initial register contents. You
can set the value of the stack pointer, the instruction pointer, and if
you want to get a little creative, you can pass some initial data to
your new thread through its registers.

<https://github.com/seL4/seL4_libs/blob/master/libsel4utils/sel4_arch_include/x86_64/sel4utils/sel4_arch/util.h>

### TASK 12

```c
/*- filter TaskContent("task-12", TaskContentType.COMPLETED, completion="main: hello world") -*/

    /* check that stack is aligned correctly */
    const int stack_alignment_requirement = sizeof(seL4_Word) * 2;
    uintptr_t thread_2_stack_top = (uintptr_t)thread_2_stack + sizeof(thread_2_stack);
    ZF_LOGF_IF(thread_2_stack_top % (stack_alignment_requirement) != 0,
               "Stack top isn't aligned correctly to a %dB boundary.\n"
               "\tDouble check to ensure you're not trampling.",
               stack_alignment_requirement);

    /* TASK 12: set stack pointer for the new thread */
    /* hint 1: sel4utils_set_stack_pointer()
     * void sel4utils_set_stack_pointer(seL4_UserContext *regs, seL4_Word value);
     * @param regs  Data structure in which to set the stack pointer value
     * @param value New stack pointer value
     * Links to source: https://docs.sel4.systems/Tutorials/seL4_Tutorial_2#task-12:
     *
     * hint 2: remember the stack grows down!
     */
    sel4utils_set_stack_pointer(&regs, thread_2_stack_top);

/*- endfilter -*/
```

This TASK is just some pointer arithmetic. The cautionary note that the
stack grows down is meant to make you think about the arithmetic.
Processor stacks push new values toward decreasing addresses, so give it
some thought.

<https://github.com/seL4/seL4_libs/blob/master/libsel4utils/sel4_arch_include/x86_64/sel4utils/sel4_arch/util.h>

### TASK 13

```c
/*- filter TaskContent("task-13", TaskContentType.COMPLETED, completion="main: hello world") -*/

    /* TASK 13: actually write the TCB registers.  We write 2 registers:
     * instruction pointer is first, stack pointer is second. */
    /* hint: seL4_TCB_WriteRegisters()
     * int seL4_TCB_WriteRegisters(seL4_TCB service, seL4_Bool resume_target, seL4_Uint8 arch_flags, seL4_Word count, seL4_UserContext *regs)
     * @param service Capability to the TCB which is being operated on.
     * @param resume_target The invocation should also resume the destination thread.
     * @param arch_flags Architecture dependent flags. These have no meaning on either IA-32 or ARM.
     * @param count The number of registers to be set.
     * @param regs Data structure containing the new register values.
     * @return 0 on success
     *
     * Note: this function is generated during build.  It is generated from the following definition:
     * Links to source: https://docs.sel4.systems/Tutorials/seL4_Tutorial_2#task-13:
     * You can find out more about it in the API manual: http://sel4.systems/Info/Docs/seL4-manual-latest.pdf
     */
    error = seL4_TCB_WriteRegisters(tcb_object.cptr, 0, 0, 2, &regs);
    ZF_LOGF_IFERR(error, "Failed to write the new thread's register set.\n"
                  "\tDid you write the correct number of registers? See arg4.\n");


/*- endfilter -*/
```

As explained above, we've been filling out our new thread's TCB for the
last few operations, so now we're writing the values we've chosen, to
the TCB object in the kernel.

<https://github.com/seL4/seL4/blob/master/libsel4/include/interfaces/sel4.xml>

### TASK 14

```c
/*- filter TaskContent("task-14", TaskContentType.COMPLETED, completion="main: hello world") -*/
    /* TASK 14: start the new thread running */
    /* hint: seL4_TCB_Resume()
     * int seL4_TCB_Resume(seL4_TCB service)
     * @param service Capability to the TCB which is being operated on.
     * @return 0 on success
     *
     * Note: this function is generated during build.  It is generated from the following definition:
     * Links to source: https://docs.sel4.systems/Tutorials/seL4_Tutorial_2#task-14:
     * You can find out more about it in the API manual: http://sel4.systems/Info/Docs/seL4-manual-latest.pdf
     */
    error = seL4_TCB_Resume(tcb_object.cptr);
    ZF_LOGF_IFERR(error, "Failed to start new thread.\n");

    /* we are done, say hello */
/*- endfilter -*/
```

Finally, we tell the kernel that our new thread is runnable. From here,
the kernel itself will choose when to run the thread based on the
priority we gave it, and according to the kernel's configured scheduling
policy.

<https://github.com/seL4/seL4/blob/master/libsel4/include/interfaces/sel4.xml>

### TASK 15

```c
void thread_2(void) {
/*- filter TaskContent("task-15", TaskContentType.COMPLETED, completion="thread_2: hallo wereld") -*/
    /* TASK 15: print something */
    /* hint: printf() */
    printf("thread_2: hallo wereld\n");
    /* never exit */
    while (1);
/*- endfilter -*/
}
```

For the sake of confirmation that our new thread was executed by the
kernel successfully, we cause it to print something to the screen.

## Globals links


- `sel4_BootInfo`:
      <https://github.com/seL4/seL4/blob/master/libsel4/include/sel4/bootinfo_types.h>
- `simple_t`:
      <https://github.com/seL4/seL4_libs/blob/master/libsel4simple/include/simple/simple.h>
- `vka_t`:
      <https://github.com/seL4/seL4_libs/blob/master//libsel4vka/include/vka/vka.h>
- `allocman_t`:
      <https://github.com/seL4/seL4_libs/blob/master/libsel4allocman/include/allocman/allocman.h>
- `name_thread()`:
      <https://github.com/SEL4PROJ/sel4-tutorials/blob/master/exercises/hello-2/src/util.c>
/*- filter ExcludeDocs() -*/

/*? ExternalFile("CMakeLists.txt") ?*/
/*? ExternalFile("src/main.c") ?*/
/*- endfilter -*/
