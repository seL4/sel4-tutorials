/*? declare_task_ordering(['threads-start','threads-configure','threads-priority',
                           'threads-context', 'threads-resume', 'threads-context-2',
                           'threads-fault']) -?*/

# Threads

This is a tutorial for using threads on seL4.

## Prerequisites

1. [Set up your machine](https://docs.sel4.systems/HostDependencies). 
2. You need to know how to address capabilities.
3. You need to know what a *root task* is. 

## Outcomes

1. Know the jargon TCB,
2. Know how to start a thread in the same address space
3. Reading and writing TCB register state
4. Know how to suspend and resume a thread
5. Thread priorities and the kernel's scheduler.
6. A brief understanding of thread exceptions and the kernel's debug fault handlers


## Background

### TCB (Thread Control Blocks)

seL4 provides threads to represent an execution context and manage processor time. A
thread is represented in seL4 by its thread control block object (TCB).
The base TCB structure contains accounting information for scheduling and IPC in addition
to the register set and floating-point context. Additionally, the TCB contains capabilities
to objects that are required for a thread to be able to run. These capabilities include
the top-level cnode, top-level page directory, IPC buffer, Fault endpoint and Reply cap slot. 
The last three objects are closely related to IPC and will be explained in more detail in a later
tutorial.

### Scheduling model

The scheduler in seL4 is used to pick which runnable thread should run next on a specific
processing core, and is a priority-based round-robin scheduler with 256 priorities (0-255).
At a scheduling decision, the kernel chooses the head of the highest-priority, non-empty
list.

#### Round robin

Kernel time is accounted for in fixed-time quanta referred to as ticks, and each TCB has
a timeslice field which represents the number of ticks that TCB is eligible to execute
until preempted. The kernel timer driver is configured to fire a periodic interrupt which
marks each tick, and when the timeslice is exhausted the thread is appended to the relevant
scheduler queue, with a replenished timeslice. Threads can surrender their current tick using
the seL4_Yield system call, which simulates timeslice exhaustion.

#### Priorities

Like any priority-based kernel without temporal isolation mechanisms, time is only guaranteed
to the highest priority threads. Priorities in seL4 act as informal capabilities: threads
cannot create threads at priorities higher than their current priority, but can create threads at
the same or lower priorities. If threads at higher priority levels never block, lower priority
threads in the system will not run. As a result, a single thread running at the highest priority
has access to 100% of processing time.

#### Domain scheduling

In order to provide confidentiality seL4 provides a top-level hierarchical scheduler which
provides static, cyclical scheduling of scheduling partitions known as domains.  Domains are
statically configured at compile time with a cyclic schedule, and are non-preemptible resulting in
completely deterministic scheduling of domains.

Threads can be assigned to domains, and threads are only scheduled
when their domain is active. Cross-domain IPC is delayed until a domain switch, and
seL4_Yield between domains is not possible. When there are no threads to run while a
domain is scheduled, a domain-specific idle thread will run until a switch occurs.

Assigning a thread to a domain requires access to the `seL4_DomainSet` capability. This allows a
thread to be added to any domain.

```c
/* Set thread's domain */
seL4_Error seL4_DomainSet_Set(seL4_DomainSet _service, seL4_Uint8 domain, seL4_TCB thread);

```

### Thread Attributes

seL4 threads are configured by invocations on the TCB object. 

#### Scheduling configuration

```c
/* libsel4 api: https://docs.sel4.systems/ApiDoc.html#sel4_tcb */

/* Scheduling attributes */
/* Suspend and resume */
seL4_Error seL4_TCB_Suspend(seL4_TCB _service)

seL4_Error seL4_TCB_Resume(seL4_TCB _service)


/* Priority and Maximum controlled priority */
seL4_Error seL4_TCB_SetPriority(seL4_TCB _service, seL4_CPtr authority, seL4_Word priority)
seL4_Error seL4_TCB_SetMCPriority(seL4_TCB _service, seL4_CPtr authority, seL4_Word mcp)
seL4_Error seL4_TCB_SetSchedParams(seL4_TCB _service, seL4_CPtr authority, seL4_Word mcp,
                                   seL4_Word priority)
/* Set affinity */
seL4_Error seL4_TCB_SetAffinity(seL4_TCB _service, seL4_Word affinity)


```

#### Execution context
```c

/* Execution context */
seL4_Error seL4_TCB_ReadRegisters(seL4_TCB _service, seL4_Bool suspend_source,
                                  seL4_Uint8 arch_flags, seL4_Word count, seL4_UserContext *regs)

seL4_Error seL4_TCB_WriteRegisters(seL4_TCB _service, seL4_Bool resume_target,
                                  seL4_Uint8 arch_flags, seL4_Word count, seL4_UserContext *regs)


```

### Thread objects 

#### Configuring CSpace, VSpace, IPC Buffer, Fault endpoint

```c
seL4_Error seL4_TCB_Configure(seL4_TCB _service, seL4_Word fault_ep, seL4_CNode cspace_root,
                              seL4_Word cspace_root_data, seL4_CNode vspace_root,
                              seL4_Word vspace_root_data, seL4_Word buffer, seL4_CPtr bufferFrame)

seL4_Error seL4_TCB_SetSpace(seL4_TCB _service, seL4_Word fault_ep, seL4_CNode cspace_root,
                             seL4_Word cspace_root_data, seL4_CNode vspace_root,
                             seL4_Word vspace_root_data)

seL4_Error seL4_TCB_SetIPCBuffer(seL4_TCB _service, seL4_Word buffer, seL4_CPtr bufferFrame)


```

#### Bound notification
```c

seL4_Error seL4_TCB_BindNotification(seL4_TCB _service, seL4_CPtr notification)

seL4_Error seL4_TCB_UnbindNotification(seL4_TCB _service)

```


## Exercises

This tutorial will guide you through using TCB invocations to create and pass variable to a new
thread in the same address space. Additionally, you will learn about how to debug a virtual memory
fault.

By the end of this tutorial you want to spawn a new thread to call this function, passing in a
third function and argument that this thread will then call. 

```c
/*-- filter TaskContent("threads-start", TaskContentType.ALL, subtask='fault') -*/

int new_thread(void *arg1, void *arg2, void *arg3) {
    printf("Hello2: arg1 %p, arg2 %p, arg3 %p\n", arg1, arg2, arg3);
    void (*func)(int) = arg1;
    func(*(int *)arg2);
    while(1);
}
/*- endfilter -*/
```

Previous tutorials have taken place in the root task where the starting CSpace layout is set by the
kernel. For this tutorial we will use the capDL-loader-app as the root task. This root task takes
a static description of all the capabilities in the system the relevant ELF binaries required to
set up a static system. It is primarily used in [Camkes](https://docs.sel4.systems/CAmkES/) projects
but we will also use it here to remove a lot of object allocation and retyping boilerplate.
The program that you construct will end up with its own CSpace and address space that are separate
from the initial seL4 root task (which will be the capdl-loader-app.

Some information about CapDL projects can be found [here](https://docs.sel4.systems/CapDL.html).

### Retype and configure a thread (vspace root, cspace roots, ipc buffer)

When you first build and run the tutorial, you should see something like the following:
```
Hello, World!
Dumping all tcbs!
Name                                        State           IP                       Prio    Core
--------------------------------------------------------------------------------------
tcb_tcb_tute                                running         0x4012ef    254                 0
idle_thread                                 idle            (nil)   0                   0
rootserver                                  inactive        0x4024c2    255                 0
<<seL4(CPU 0) [decodeInvocation/530 T0xffffff8008140c00 "tcb_tcb_tute" @4012ef]: Attempted to invoke a >
main@tcb_tute.c:42 [Cond failed: result]
/*-- filter TaskCompletion("threads-start", TaskContentType.ALL) -*/
Failed to retype thread: 2
/*- endfilter --*/
```

This is generated by the source below. It should be clear that `Hello, World!` comes from `printf`.
`Dumping all tcbs!` and the following table is generated by a debug syscall called `seL4_DebugDumpScheduler()`.
seL4 has a series of debug syscalls that are available in debug kernel builds. The available debug syscalls
can be found in [libsel4](https://docs.sel4.systems/ApiDoc.html#debugging-system-calls). `seL4_DebugDumpScheduler()`
is used to dump the current state of the scheduler and can be useful to debug situations where a
system seems to have hung.  Finally, our `seL4_Untyped_Retype` invocation is failing due to invalid
arguments.

The goal for this task is to create a TCB object and configure it to have the same CSpace and VSpace
as the current thread. The loader has been configured to create capabilities for the following objects:
- `root_cnode`: Cap for the root CSpace of the current thread
- `root_vspace`: Cap for the root VSpace of the current thread
- `root_tcb`: Cap for the TCB of the current thread
- `tcb_untyped`: Untyped object large enough to create a new TCB object
- `tcb_cap_slot`: Empty slot for the new TCB object
- `tcb_ipc_frame`: Cap for a frame mapping to be used as the IPC Buffer

Additionally there are two page mappings that have been created:
- `thread_ipc_buff_sym`: Symbol for the IPC buffer in the address space
- `tcb_stack_top`: Symbol for the top of a 16 * 4KiB stack mapping

You can use the above objects to create a new TCB object referred to by a cap in `tcb_cap_slot`.
Once the thread has been created it will show up in the `seL4_DebugDumpScheduler()` output. Throughout
the tutorial you can use this syscall to debug some of the TCB attributes that you set.

Once the TCB has been created, the next step is to configure it with a CSpace, VSpace and IPC buffer.
We don't set a fault handler as the kernel will print any fault we receive with a debug build.
Use `seL4_TCB_Configure` to configure the TCB object.

```c
/*-- filter TaskContent("threads-start", TaskContentType.ALL, subtask='configure') -*/
int main(int c, char* arbv[]) {

    printf("Hello, World!\n");

    seL4_DebugDumpScheduler();
    seL4_Error result = seL4_Untyped_Retype(seL4_CapNull, seL4_TCBObject, seL4_TCBBits, seL4_CapNull, seL4_CapNull, 0, seL4_CapNull, 1);
    ZF_LOGF_IF(result, "Failed to retype thread: %d", result);
    seL4_DebugDumpScheduler();

    result = seL4_TCB_Configure(seL4_CapNull, seL4_CapNull, 0, seL4_CapNull, 0, 0, (seL4_Word) NULL, seL4_CapNull);
    ZF_LOGF_IF(result, "Failed to configure thread: %d", result);

/*-- endfilter -*/
```
/*- filter ExcludeDocs() -*/

```c
/*-- filter TaskContent("threads-configure", TaskContentType.COMPLETED, subtask='configure', completion="child of: 'tcb_tcb_tute'") -*/
int main(int c, char* arbv[]) {

    printf("Hello, World!\n");

    seL4_DebugDumpScheduler();
    seL4_Error result = seL4_Untyped_Retype(thread_untyped, seL4_TCBObject, seL4_TCBBits, root_cnode, tcb_cap_slot, 0, tcb_cap_slot, 1);
    ZF_LOGF_IF(result, "Failed to retype thread: %d", result);
    seL4_DebugDumpScheduler();

    result = seL4_TCB_Configure(tcb_cap_slot, seL4_CapNull, root_cnode, 0, root_vspace, 0, (seL4_Word) thread_ipc_buff_sym, tcb_ipc_frame);
    ZF_LOGF_IF(result, "Failed to configure thread: %d", result);

/*-- endfilter -*/
```
/*- endfilter -*/

You should now be getting the following error:
```
<<seL4(CPU 0) [decodeSetPriority/1035 T0xffffff8008140c00 "tcb_tcb_tute" @4012ef]: Set priority: author>
main@tcb_tute.c:51 [Cond failed: result]
/*-- filter TaskCompletion("threads-priority", TaskContentType.BEFORE) -*/
Failed to set the priority for the new TCB object.
/*- endfilter --*/
```

### Change a threads priority via `seL4_TCB_SetPriority`

A newly created thread will have a priority of 0. In order for it to run, it will need to have its
priority increased such that it will eventually end up being a highest priority runnable thread.
You need to use `seL4_TCB_SetPriority` to set the priority. Remember that to set a thread's priority,
the calling thread must have the authority to do so. The main thread has a priority of 254 and is
configured with the same maximum controlled priority attribute.


```c
/*-- filter TaskContent("threads-start", TaskContentType.ALL, subtask='priority') -*/
    result = seL4_TCB_SetPriority(tcb_cap_slot, seL4_CapNull, 0);
    ZF_LOGF_IF(result, "Failed to set the priority for the new TCB object.\n");
    seL4_DebugDumpScheduler();
/*-- endfilter -*/
```
/*- filter ExcludeDocs() -*/

```c
/*-- filter TaskContent("threads-priority", TaskContentType.COMPLETED, subtask='priority') -*/
    result = seL4_TCB_SetPriority(tcb_cap_slot, root_tcb, 254);
    ZF_LOGF_IF(result, "Failed to set the priority for the new TCB object.\n");
    seL4_DebugDumpScheduler();
/*-- endfilter -*/
```

/*- endfilter -*/

Fixing up the `seL4_TCB_SetPriority` call should allow you to see that the thread's priority is now
set to the same as the main thread in the next `seL4_DebugDumpScheduler()` call.

```
Name                                        State           IP                       Prio    Core
--------------------------------------------------------------------------------------
child of: 'tcb_tcb_tute'                    inactive        (nil)   254                 0
tcb_tcb_tute                                running         0x4012ef    254                 0
idle_thread                                 idle            (nil)   0                   0
rootserver                                  inactive        0x4024c2    255                 0
<<seL4(CPU 0) [decodeInvocation/530 T0xffffff8008140c00 "tcb_tcb_tute" @4012ef]: Attempted to invoke a >
main@tcb_tute.c:57 [Err seL4_InvalidCapability]:
/*-- filter TaskCompletion("threads-priority", TaskContentType.COMPLETED) -*/
Failed to read the new thread's register set.
/*- endfilter --*/
```

### Set initial register state (a stack and initial program counter, arguments)

The only thing left before starting a thread is to set its initial registers. You need to set the
program counter and stack pointer in order for the thread to be able to run without immediately
crashing.

`libsel4utils` contains some functions for setting register contents in a platform agnostic manner.
You can use these methods to set the program counter (instruction pointer) and stack pointer in
this way.  _Note: It is assumed that the stack grows downwards on all platforms._

Set up the new thread to call `new_thread`.  You can use the debug syscall to verify that
you have at least set the instruction pointer (IP) correctly.


```c
/*-- filter TaskContent("threads-start", TaskContentType.ALL, subtask='context') -*/
    UNUSED seL4_UserContext regs = {0};
    int error = seL4_TCB_ReadRegisters(seL4_CapNull, 0, 0, 0, &regs);
    ZF_LOGF_IFERR(error, "Failed to read the new thread's register set.\n");

    sel4utils_set_instruction_pointer(&regs, (seL4_Word)NULL);
    sel4utils_set_stack_pointer(&regs, NULL);
    error = seL4_TCB_WriteRegisters(seL4_CapNull, 0, 0, 0, &regs);
    ZF_LOGF_IFERR(error, "Failed to write the new thread's register set.\n");
    seL4_DebugDumpScheduler();

/*-- endfilter -*/
```
/*- filter ExcludeDocs() -*/

```c
/*-- filter TaskContent("threads-context", TaskContentType.COMPLETED, subtask='context', completion='Failed to start new thread') -*/
    UNUSED seL4_UserContext regs = {0};
    int error = seL4_TCB_ReadRegisters(tcb_cap_slot, 0, 0, sizeof(regs)/sizeof(seL4_Word), &regs);
    ZF_LOGF_IFERR(error, "Failed to write the new thread's register set.\n"
                  "\tDid you write the correct number of registers? See arg4.\n");

    sel4utils_set_instruction_pointer(&regs, (seL4_Word)new_thread);
    sel4utils_set_stack_pointer(&regs, tcb_stack_top);
    error = seL4_TCB_WriteRegisters(tcb_cap_slot, 0, 0, sizeof(regs)/sizeof(seL4_Word), &regs);
    ZF_LOGF_IFERR(error, "Failed to write the new thread's register set.\n"
                  "\tDid you write the correct number of registers? See arg4.\n");
    seL4_DebugDumpScheduler();

/*-- endfilter -*/
```
/*- endfilter -*/

### Resume a thread

If everything has been configured correctly, resuming the thread should result in a new print
output.

```c
/*-- filter TaskContent("threads-start", TaskContentType.ALL, subtask='resume') -*/

    error = seL4_TCB_Resume(seL4_CapNull);
    ZF_LOGF_IFERR(error, "Failed to start new thread.\n");


/*-- endfilter -*/
```
/*- filter ExcludeDocs() -*/

```c
/*-- filter TaskContent("threads-resume", TaskContentType.COMPLETED, subtask='resume', completion='Hello2: arg1 0, arg2 0, arg3 0') -*/

    error = seL4_TCB_Resume(tcb_cap_slot);
    ZF_LOGF_IFERR(error, "Failed to start new thread.\n");

/*-- endfilter -*/
```
/*- endfilter -*/

### Set arguments directly, or use a function to abstract over achitecture.

You will notice that all of the arguments to the new thread are 0. We can use another helper function
to set the registers to some dummy values. Changing the argument registers will change the arguments
that the new thread's function will be called with. If you want, you can try setting the exact
registers in the seL4_UserContext for your chosen architecture.

```c
/*-- filter TaskContent("threads-context-2", TaskContentType.ALL, subtask='context', completion='Hello2: arg1 0x1, arg2 0x2, arg3 0x3') -*/
    UNUSED seL4_UserContext regs = {0};
    int error = seL4_TCB_ReadRegisters(tcb_cap_slot, 0, 0, sizeof(regs)/sizeof(seL4_Word), &regs);
    ZF_LOGF_IFERR(error, "Failed to write the new thread's register set.\n"
                  "\tDid you write the correct number of registers? See arg4.\n");


    sel4utils_arch_init_local_context((void*)new_thread,
                                  (void *)1, (void *)2, (void *)3,
                                  (void *)tcb_stack_top, &regs);
    error = seL4_TCB_WriteRegisters(tcb_cap_slot, 0, 0, sizeof(regs)/sizeof(seL4_Word), &regs);
    ZF_LOGF_IFERR(error, "Failed to write the new thread's register set.\n"
                  "\tDid you write the correct number of registers? See arg4.\n");
/*- endfilter -*/
```
### Resolving a thread fault

At this point, you have created and configured a new thread, and given it initial arguments.
The last part of this tutorial is what to do when your thread faults. In another tutorial you
can learn how to recover from faulting threads using a fault handler, but for now you can rely on the
kernel printing a fault message for any faulting thread that doesn't have a fault handler set.

In the output below you can see a cap fault has occurred. The first part of the error is that
the kernel was unable to send a fault to a fault handler as it is set to `(nil)`. The kernel
then prints out the fault it was trying to send. It is a virtual memory fault. The error indicates
that a child thread of thread `tcb_tcb_tute` tried to access data at address `0x2` which does not
have a mapping set up. The program counter of this thread was `0x401e66`. It is further possible
to look up the status of the fault in the relevant architecture manual for your platform to decode
what sort of data fault occurred. Additionally, the kernel prints a literal stack trace with the top
being the current value of the stack pointer.  The kernel can be configured to print out more or
less items in the trace by changing `KernelUserStackTraceLength`. By default it is 16.

```
Caught cap fault in send phase at address (nil)
while trying to handle:
vm fault on data at address 0x2 with status 0x4
in thread 0xffffff8008140400 "child of: 'tcb_tcb_tute'" at address 0x401e66
With stack:
0x439fc0: 0x0
0x439fc8: 0x3
0x439fd0: 0x2
0x439fd8: 0x1
0x439fe0: 0x0
0x439fe8: 0x1
0x439ff0: 0x0
0x439ff8: 0x0
0x43a000: 0x404fb3
0x43a008: 0x0
0x43a010: 0x0
0x43a018: 0x0
0x43a020: 0x0
0x43a028: 0x0
0x43a030: 0x0
0x43a038: 0x0
```

Once you have this fault information you can look what happened by using a tool such as `objdump`
on the ELF file that was loaded. In our case, the ELF file is located at `./<BUILD_DIR>/<TUTORIAL_BUILD_DIR>/tcb_tute`.
(In normal project structures, the location of the ELF will be in the same place as its source directory, but in the
build directory.)

By looking up the faulting IP you can see that `arg2` is being dereferenced to be passed to a function
call. Fixing this fault will require initialising `arg2` to some valid memory. Set it to some memory
location controlled by the main thread, such as the address of a global variable. Stack variables
wont work here.

You likely noticed that the new thread also tries to dereference `arg1` as a function to call. Letting
this happen will lead to a different vm fault type which you can test out.  Fixing this will require
creating a new function to pass in from the main thread.

### Further exercises

That's it. The tutorial is finished. For further exercises you could try:
- Using different TCB invocations to change the new thread's attributes or objects
- Investigate how setting different priorities affects when the threads are scheduled to run
- Implementing synchronisation primitives using global memory.
- Trying to repeat this tutorial in the root task where there are more resources available to
  create more thread objects.
- Another tutorial...

/*? macros.help_block() ?*/

/*- filter ExcludeDocs() -*/

```c
/*-- filter TaskContent("threads-fault", TaskContentType.COMPLETED, subtask='fault') -*/

int data = 42;

int new_thread(void *arg1, void *arg2, void *arg3) {
    printf("Hello2: arg1 %p, arg2 %p, arg3 %p\n", arg1, arg2, arg3);
    void (*func)(int) = arg1;
    func(*(int *)arg2);
    while(1);
}
/*- endfilter -*/
```

```c
/*-- filter TaskContent("threads-fault", TaskContentType.COMPLETED, subtask='func2', completion='Hello 3 42') -*/
int call_once(int arg) {
    printf("Hello 3 %d\n", arg);
}
/*- endfilter -*/
```

```c
/*-- filter TaskContent("threads-fault", TaskContentType.COMPLETED, subtask='context') -*/
    UNUSED seL4_UserContext regs = {0};
    int error = seL4_TCB_ReadRegisters(tcb_cap_slot, 0, 0, sizeof(regs)/sizeof(seL4_Word), &regs);
    ZF_LOGF_IFERR(error, "Failed to write the new thread's register set.\n"
                  "\tDid you write the correct number of registers? See arg4.\n");


    sel4utils_arch_init_local_context((void*)new_thread,
                                  (void *)call_once, (void *)&data, (void *)3,
                                  
                                  (void *)tcb_stack_top, &regs);
    error = seL4_TCB_WriteRegisters(tcb_cap_slot, 0, 0, sizeof(regs)/sizeof(seL4_Word), &regs);
    ZF_LOGF_IFERR(error, "Failed to write the new thread's register set.\n"
                  "\tDid you write the correct number of registers? See arg4.\n");
/*- endfilter -*/
```

```c
/*- set progname = "tcb_tute" -*/
/*- filter ELF(progname) --*/

#include <stdio.h>
#include <sel4/sel4.h>
#include <utils/util.h>
#include <sel4utils/util.h>
#include <sel4utils/helpers.h>

/*? capdl_my_cspace(progname, "root_cnode") ?*/
/*? capdl_my_vspace(progname, "root_vspace") ?*/
/*? capdl_my_tcb(progname, "root_tcb") ?*/

/*? RecordObject(seL4_UntypedObject, "tcb_untyped", cap_symbol="thread_untyped", size_bits= 11)?*/

/*? capdl_empty_slot("tcb_cap_slot") ?*/

/*? capdl_declare_ipc_buffer("tcb_ipc_frame", "thread_ipc_buff_sym") ?*/
/*? capdl_declare_stack(16 * 4096, "tcb_stack_base", "tcb_stack_top") ?*/

/*? include_task_type_append([("threads-fault", 'func2')]) ?*/


/*? include_task_type_replace([("threads-start", 'fault'), ("threads-fault", 'fault')]) ?*/


/*? include_task_type_replace([("threads-start", 'configure'), ("threads-configure", 'configure')]) ?*/

/*? include_task_type_replace([("threads-start", 'priority'), ("threads-priority", 'priority')]) ?*/

/*? include_task_type_replace([("threads-start", 'context'), ("threads-context", 'context'),
                              ("threads-context-2", 'context'), ("threads-fault", 'context')]) ?*/

/*? include_task_type_replace([("threads-start", 'resume'), ("threads-resume", 'resume')]) ?*/

    while(1);
    return 0;
}

/*- endfilter -*/
```

```cmake
/*-- filter File("CMakeLists.txt") -*/
ImportCapDL()
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -u __vsyscall_ptr")
/*? write_manifest(".manifest.obj") ?*/
/*- for (elf, file) in state.stash.elfs.iteritems() --*/
cdl_pp(${CMAKE_CURRENT_SOURCE_DIR}/.manifest.obj ${CMAKE_CURRENT_BINARY_DIR}/manifest_/*?elf?*/.p /*?elf?*/_pp
    ELF "/*?elf?*/"
    CFILE "${CMAKE_CURRENT_BINARY_DIR}/cspace_/*?elf?*/.c"
)   

add_executable(/*?elf?*/ EXCLUDE_FROM_ALL /*?file['filename']?*/ cspace_/*?elf?*/.c )
add_dependencies(/*?elf?*/ /*?elf?*/_pp)
target_link_libraries(/*?elf?*/ sel4
    muslc utils sel4tutorials
    sel4muslcsys sel4platsupport sel4utils sel4debug)

list(APPEND elf_files "$<TARGET_FILE:/*?elf?*/>")
list(APPEND elf_targets "/*?elf?*/")
list(APPEND manifests ${CMAKE_CURRENT_BINARY_DIR}/manifest_/*?elf?*/.p)

/*- endfor --*/


cdl_ld("${CMAKE_CURRENT_BINARY_DIR}/spec.cdl" capdl_spec 
    MANIFESTS ${manifests}
    ELF ${elf_files}
    DEPENDS ${elf_targets})

DeclareCDLRootImage("${CMAKE_CURRENT_BINARY_DIR}/spec.cdl" capdl_spec ELF ${elf_files} ELF_DEPENDS ${elf_targets})

/*? macros.cmake_check_script(state) ?*/
/*- endfilter -*/
```
/*- endfilter -*/
