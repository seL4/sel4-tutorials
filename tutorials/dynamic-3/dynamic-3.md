<!--
  Copyright 2017, Data61, CSIRO (ABN 41 687 119 230)

  SPDX-License-Identifier: BSD-2-Clause
-->

/*? declare_task_ordering(
['task-1',
'task-2',
'task-3',
'task-4',
'task-5',
'task-6',
'task-7',
'task-8'
])?*/

# seL4 Dynamic Libraries: Processes & Elf loading

This tutorial shows how a separate ELF file can be loaded and expanded into a
VSpace, and subsequently executed, while facilitating IPC between the
two modules. It also shows how threads with different CSpaces have to
maneuver in order to pass capabilities to one another.

Don't gloss over the globals declared before `main()` -- they're declared
for your benefit so you can grasp some of the basic data structures.
Uncomment them one by one as needed when going through the tasks.

You'll observe that the things you've already covered in the second
tutorial are filled out and you don't have to repeat them: in much the
same way, we won't be repeating conceptual explanations on this page, if
they were covered by a previous tutorial in the series.

## Learning outcomes

- Once again, repeat the spawning of a thread: however, this time
        the two threads will only share the same vspace, but have
        different CSpaces. This is an automatic side effect of the way
        that sel4utils creates new "processes".
- Learn how the init thread in an seL4 system performs some types
        of initialization that aren't traditionally left to userspace.
- Observe and understand the effects of creating thread that do
        not share the same CSpace.
- Understand IPC across CSpace boundaries.
- Understand how minting a capability to a thread in another
        CSpace works.


## Initialising

/*? macros.tutorial_init("dynamic-3") ?*/

## Prerequisites

1. [Set up your machine](https://docs.sel4.systems/HostDependencies).
1. [dynamic-2](https://docs.sel4.systems/Tutorials/dynamic-2)

## Exercises

Tasks in this tutorial are in `main.c` and `app.c`.

When you first run this tutorial, you should see the following output:

```
Booting all finished, dropped to user space
Node 0 of 1
IOPT levels:     4294967295
IPC buffer:      0x57f000
Empty slots:     [488 --> 4096)
sharedFrames:    [0 --> 0)
userImageFrames: [16 --> 399)
userImagePaging: [12 --> 15)
untypeds:        [399 --> 488)
Initial thread domain: 0
Initial thread cnode size: 12
dynamic-3: vspace_reserve_range_aligned@vspace.h:621 Not implemented
dynamic-3: main@main.c:117 [Cond failed: virtual_reservation.res == NULL]
/*-- filter TaskCompletion("task-1", TaskContentType.BEFORE) -*/
	Failed to reserve a chunk of memory.
/*-- endfilter -*/
```

### Virtual memory management

Aside from receiving information about IRQs in the IRQControl object
capability, and information about available IO-Ports, and ASID
availability, and several other privileged bits of information, the init
thread is also responsible, surprisingly, for reserving certain critical
ranges of memory as being used, and unavailable for applications.

This call to `sel4utils_bootstrap_vspace_with_bootinfo_leaky()` does
that. For an interesting look at what sorts of things the init thread
does, see:
`static int reserve_initial_task_regions(vspace_t *vspace, void *existing_frames[])`,
which is eventually called on by
`sel4utils_bootstrap_vspace_with_bootinfo_leaky()`. So while this
function may seem tedious, it's doing some important things.

- <https://github.com/seL4/seL4_libs/blob/master/libsel4utils/include/sel4utils/vspace.h>
- <https://github.com/seL4/seL4_libs/blob/master/libsel4utils/include/sel4utils/vspace.h>

```c
/*-- set task_1_desc -*/
    /* TASK 1: create a vspace object to manage our vspace */
    /* hint 1: sel4utils_bootstrap_vspace_with_bootinfo_leaky()
     * int sel4utils_bootstrap_vspace_with_bootinfo_leaky(vspace_t *vspace, sel4utils_alloc_data_t *data, seL4_CPtr page_directory, vka_t *vka, seL4_BootInfo *info)
     * @param vspace Uninitialised vspace struct to populate.
     * @param data Uninitialised vspace data struct to populate.
     * @param page_directory Page directory for the new vspace.
     * @param vka Initialised vka that this virtual memory allocator will use to allocate pages and pagetables. This allocator will never invoke free.
     * @param info seL4 boot info
     * @return 0 on succes.
     */
/*-- endset -*/
/*? task_1_desc ?*/
/*-- filter TaskContent("task-1", TaskContentType.BEFORE) -*/
/*-- endfilter -*/
/*-- filter ExcludeDocs() -*/
/*-- filter TaskContent("task-1", TaskContentType.COMPLETED) -*/
    error = sel4utils_bootstrap_vspace_with_bootinfo_leaky(&vspace,
                                                           &data, simple_get_pd(&simple), &vka, info);
/*-- endfilter -*/
/*-- endfilter -*/
```

On success, you should see a different error:

```
<<seL4(CPU 0) [handleUnknownSyscall/106 T0xffffff801ffb5400 "dynamic-3" @40139e]: SysDebugNameThread: cap is not a TCB, halting>>
/*-- filter TaskCompletion("task-1", TaskContentType.COMPLETED) -*/
halting...
/*-- endfilter -*/
```

### Configure a process

`sel4utils_configure_process_custom` took a large amount of the work
out of creating a new "processs". We skipped a number of steps. Take a
look at the source for `sel4utils_configure_process_custom()` and
notice how it spawns the new thread with its own CSpace by
automatically. This will have an effect on our tutorial! It means that
the new thread we're creating will not share a CSpace with our main
thread.

- <https://github.com/seL4/seL4_libs/blob/master/libsel4utils/include/sel4utils/process.h>

```c
/*-- set task_2_desc -*/
    /* TASK 2: use sel4utils to make a new process */
    /* hint 1: sel4utils_configure_process_custom()
     * hint 2: process_config_default_simple()
     * @param process Uninitialised process struct.
     * @param vka Allocator to use to allocate objects.
     * @param vspace Vspace allocator for the current vspace.
     * @param priority Priority to configure the process to run as.
     * @param image_name Name of the elf image to load from the cpio archive.
     * @return 0 on success, -1 on error.
     *
     * hint 2: priority is in APP_PRIORITY and can be 0 to seL4_MaxPrio
     * hint 3: the elf image name is in APP_IMAGE_NAME
     */
/*-- endset -*/
/*? task_2_desc ?*/
/*-- filter ExcludeDocs() -*/
/*-- filter TaskContent("task-2", TaskContentType.COMPLETED) -*/
    sel4utils_process_config_t config = process_config_default_simple(&simple, APP_IMAGE_NAME, APP_PRIORITY);
    error = sel4utils_configure_process_custom(&new_process, &vka, &vspace, config);
/*-- endfilter -*/
/*-- endfilter -*/
```

On success, you should see a different error:

```
 dynamic-3: main@main.c:196 [Cond failed: new_ep_cap == 0]
/*-- filter TaskCompletion("task-2", TaskContentType.COMPLETED) -*/
	Failed to mint a badged copy of the IPC endpoint into the new thread's CSpace.
	sel4utils_mint_cap_to_process takes a cspacepath_t: double check what you passed.
/*-- endfilter -*/
```

### Get a `cspacepath`

Now, in this particular case, we are making the new thread be the
sender. Recall that the sender must have a capability to the endpoint
that the receiver is listening on, in order to send to that listener.
But in this scenario, our threads do **not** share the same CSpace!
The only way the new thread will know which endpoint it needs a
capability to, is if we tell it. Furthermore, even if the new thread
knows which endpoint object we are listening on, if it doesn't have a
capability to that endpoint, it still can't send data to us. So we must
provide our new thread with both a capability to the endpoint we're
listening on, and also make sure, that that capability we give it has
sufficient privileges to send across the endpoint.

There is a number of ways we could approach this, but in this tutorial
we decided to just pre-initialize the sender's CSpace with a sufficient
capability to enable it to send to us right from the start of its
execution. We could also have spawned the new thread as a listener
instead, and made it wait for us to send it a message with a sufficient
capability.

So we use `vka_cspace_make_path()`, which locates one free capability
slot in the selected CSpace, and returns a handle to it, to us. We then
filled that free slot in the new thread's CSpace with a **badged**
capability to the endpoint we are listening on, so as so allow it to
send to us immediately. We could have filled the slot with an unbadged
capability, but then if we were listening for multiple senders, we
wouldn't know who was whom.

- <https://github.com/seL4/seL4_libs/blob/master/libsel4vka/include/vka/vka.h>

```c
/*-- set task_3_desc -*/
    /* TASK 3: make a cspacepath for the new endpoint cap */
    /* hint 1: vka_cspace_make_path()
     * void vka_cspace_make_path(vka_t *vka, seL4_CPtr slot, cspacepath_t *res)
     * @param vka Vka interface to use for allocation of objects.
     * @param slot A cslot allocated by the cspace alloc function
     * @param res Pointer to a cspacepath struct to fill out
     *
     * hint 2: use the cslot of the endpoint allocated above
     */
    cspacepath_t ep_cap_path;
    seL4_CPtr new_ep_cap = 0;
/*-- endset -*/
/*? task_3_desc ?*/
/*-- filter ExcludeDocs() -*/
/*-- filter TaskContent("task-3", TaskContentType.COMPLETED, completion="what you passed.") -*/
    vka_cspace_make_path(&vka, ep_object.cptr, &ep_cap_path);
/*-- endfilter -*/
/*-- endfilter -*/
 ```
On success, the output should not change.

### Badge a capability

As discussed above, we now just mint a badged copy of a capability to
the endpoint we're listening on, into the new thread's CSpace, in the
free slot that the VKA library found for us.

- <https://github.com/seL4/seL4_libs/blob/master/libsel4utils/include/sel4utils/process.h>
- <https://github.com/seL4/seL4/blob/master/libsel4/mode_include/32/sel4/shared_types.bf>

```c
/*-- set task_4_desc -*/
    /* TASK 4: copy the endpont cap and add a badge to the new cap */
    /* hint 1: sel4utils_mint_cap_to_process()
     * seL4_CPtr sel4utils_mint_cap_to_process(sel4utils_process_t *process, cspacepath_t src, seL4_CapRights rights, seL4_Word data)
     * @param process Process to copy the cap to
     * @param src Path in the current cspace to copy the cap from
     * @param rights The rights of the new cap
     * @param data Extra data for the new cap (e.g., the badge)
     * @return 0 on failure, otherwise the slot in the processes cspace.
     *
     * hint 2: for the rights, use seL4_AllRights
     * hint 3: for the badge value use EP_BADGE
     */
/*-- endset -*/
/*? task_4_desc ?*/
/*-- filter ExcludeDocs() -*/
/*-- filter TaskContent("task-4", TaskContentType.COMPLETED) -*/
    new_ep_cap = sel4utils_mint_cap_to_process(&new_process, ep_cap_path,
                                               seL4_AllRights, EP_BADGE);
/*-- endfilter -*/
/*-- endfilter -*/
```

On success, the output should look something like:

```
NEW CAP SLOT: 6ac.
main: hello world
dynamic-3: main@main.c:247 [Cond failed: sender_badge != EP_BADGE]
/*-- filter TaskCompletion("task-4", TaskContentType.COMPLETED) -*/
	The badge we received from the new thread didn't match our expectation
/*-- endfilter -*/
```

### Spawn a process

So now that we've given the new thread everything it needs to
communicate with us, we can let it run. Complete this step and proceed.

- <https://github.com/seL4/seL4_libs/blob/master/libsel4utils/include/sel4utils/process.h>

```c
/*-- set task_5_desc -*/
    /* TASK 5: spawn the process */
    /* hint 1: sel4utils_spawn_process_v()
     * int sel4utils_spawn_process_v(sel4utils_process_t *process, vka_t *vka, vspace_t *vspace, int argc, char *argv[], int resume)
     * @param process Initialised sel4utils process struct.
     * @param vka Vka interface to use for allocation of frames.
     * @param vspace The current vspace.
     * @param argc The number of arguments.
     * @param argv A pointer to an array of strings in the current vspace.
     * @param resume 1 to start the process, 0 to leave suspended.
     * @return 0 on success, -1 on error.
     */
    /* hint 2: sel4utils_create_word_args()
     * void sel4utils_create_word_args(char strings[][WORD_STRING_SIZE], char *argv[], int argc, ...)
     * Create c-formatted argument list to pass to a process from arbitrarily long amount of words.
     *
     * @param strings empty 2d array of chars to populate with word strings.
     * @param argv empty 1d array of char pointers which will be set up with pointers to
     *             strings in strings.
     * @param argc number of words
     * @param ... list of words to create arguments from.
     *
     */

/*-- endset -*/
/*? task_5_desc ?*/
/*-- filter ExcludeDocs() -*/
/*-- filter TaskContent("task-5", TaskContentType.COMPLETED) -*/
    new_ep_cap = sel4utils_mint_cap_to_process(&new_process, ep_cap_path,
                                               seL4_AllRights, EP_BADGE);
    seL4_Word argc = 1;
    char string_args[argc][WORD_STRING_SIZE];
    char* argv[argc];
    sel4utils_create_word_args(string_args, argv, argc, new_ep_cap);

    error = sel4utils_spawn_process_v(&new_process, &vka, &vspace, argc, (char**) &argv, 1);
/*-- endfilter -*/
/*-- endfilter -*/
```

On success, you should be able to see the second process running. The output should
be as follows:

```
NEW CAP SLOT: 6ac.
process_2: hey hey hey
main@app.c:67 [Cond failed: msg != ~MSG_DATA]
/*-- filter TaskCompletion("task-5", TaskContentType.COMPLETED) -*/
	Unexpected response from root thread.
/*-- endfilter -*/
main: hello world
dynamic-3: main@main.c:255 [Cond failed: sender_badge != EP_BADGE]
	The badge we received from the new thread didn't match our expectation.
```

### Receive a message

We now wait for the new thread to send us data using `seL4_Recv()`...
Then we verify the fidelity of the data that was transmitted.

- <https://github.com/seL4/seL4/blob/master/libsel4/sel4_arch_include/ia32/sel4/sel4_arch/syscalls.h>
- <https://github.com/seL4/seL4/blob/master/libsel4/mode_include/32/sel4/shared_types.bf>

```c
/*-- set task_6_desc -*/
    /* TASK 6: wait for a message */
    /* hint 1: seL4_Recv()
     * seL4_MessageInfo_t seL4_Recv(seL4_CPtr src, seL4_Word* sender)
     * @param src The capability to be invoked.
     * @param sender The badge of the endpoint capability that was invoked by the sender is written to this address.
     * @return A seL4_MessageInfo_t structure
     *
     * hint 2: seL4_MessageInfo_t is generated during build.
     * hint 3: use the badged endpoint cap that you minted above
     */
/*-- endset -*/
/*? task_6_desc ?*/
/*-- filter ExcludeDocs() -*/
/*-- filter TaskCompletion("task-6", TaskContentType.COMPLETED) -*/
	Unexpected response from root thread.
/*-- endfilter -*/
/*-- filter TaskContent("task-6", TaskContentType.COMPLETED) -*/
    tag = seL4_Recv(ep_cap_path.capPtr, &sender_badge);
/*-- endfilter -*/
/*-- endfilter -*/
```

On success, the badge error should no longer be visible.

### Send a reply

Another demonstration of the `sel4_Reply()` facility: we reply to the
message sent by the new thread.

- <https://github.com/seL4/seL4/blob/master/libsel4/sel4_arch_include/ia32/sel4/sel4_arch/syscalls.h#L621>
- <https://github.com/seL4/seL4/blob/master/libsel4/mode_include/32/sel4/shared_types.bf#L11>

```c
/*-- set task_7_desc -*/
    /* TASK 7: send the modified message back */
    /* hint 1: seL4_ReplyRecv()
     * seL4_MessageInfo_t seL4_ReplyRecv(seL4_CPtr dest, seL4_MessageInfo_t msgInfo, seL4_Word *sender)
     * @param dest The capability to be invoked.
     * @param msgInfo The messageinfo structure for the IPC.  This specifies information about the message to send (such as the number of message registers to send) as the Reply part.
     * @param sender The badge of the endpoint capability that was invoked by the sender is written to this address. This is a result of the Wait part.
     * @return A seL4_MessageInfo_t structure.  This is a result of the Wait part.
     *
     * hint 2: seL4_MessageInfo_t is generated during build.
     * hint 3: use the badged endpoint cap that you used for Call
     */
/*-- endset -*/
/*? task_7_desc ?*/
/*-- filter ExcludeDocs() -*/
/*-- filter TaskCompletion("task-7", TaskContentType.COMPLETED) -*/
	Unexpected response from root thread.
/*-- endfilter -*/
/*-- filter TaskContent("task-7", TaskContentType.COMPLETED) -*/
    seL4_ReplyRecv(ep_cap_path.capPtr, tag, &sender_badge);
/*-- endfilter -*/
/*-- endfilter -*/
```

On success, the output should not change.

### Client Call

In the new thread, we initiate communications by using `seL4_Call()`. As
outlined above, the receiving thread replies to us using
`sel4_ReplyRecv()`. The new thread then checks the fidelity of the data
that was sent, and that's the end.

- <https://github.com/seL4/seL4/blob/master/libsel4/sel4_arch_include/ia32/sel4/sel4_arch/syscalls.h>

```c
/*-- set task_8_desc -*/
 /* TASK 8: send and wait for a reply */
    /* hint 1: seL4_Call()
     * seL4_MessageInfo_t seL4_Call(seL4_CPtr dest, seL4_MessageInfo_t msgInfo)
     * @param dest The capability to be invoked.
     * @param msgInfo The messageinfo structure for the IPC.  This specifies information about the message to send (such as the number of message registers to send).
     * @return A seL4_MessageInfo_t structure.  This is information about the repy message.
     *
     * hint 2: send the endpoint cap using argv (see TASK 6 in the other main.c)
     */
    ZF_LOGF_IF(argc < 1,
               "Missing arguments.\n");
    seL4_CPtr ep = (seL4_CPtr) atol(argv[0]);
/*-- endset -*/
/*? task_8_desc ?*/
/*-- filter ExcludeDocs() -*/
/*-- filter TaskContent("task-8", TaskContentType.COMPLETED) -*/
    tag = seL4_Call(ep, tag);

/*-- endfilter -*/
/*-- endfilter -*/
```

On success, you should see the following:

```
/*-- filter TaskCompletion("task-8", TaskContentType.COMPLETED) -*/
process_2: hey hey hey
main: got a message 0x6161 from 0x61
/*-- endfilter -*/
process_2: got a reply: 0xffffffffffff9e9e
```

That's it for this tutorial.

/*? macros.help_block() ?*/
/*-- filter ExcludeDocs() -*/
/*? ExternalFile("CMakeLists.txt") ?*/
```
/*-- filter File("main.c") -*/
/*
 * Copyright 2017, Data61, CSIRO (ABN 41 687 119 230).
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */
#include <autoconf.h>

#include <stdio.h>
#include <assert.h>

#include <sel4/sel4.h>

#include <simple/simple.h>
#include <simple-default/simple-default.h>

#include <vka/object.h>

#include <allocman/allocman.h>
#include <allocman/bootstrap.h>
#include <allocman/vka.h>

#include <vspace/vspace.h>

#include <sel4utils/vspace.h>
#include <sel4utils/mapping.h>
#include <sel4utils/process.h>

#include <utils/arith.h>
#include <utils/zf_log.h>
#include <sel4utils/sel4_zf_logif.h>

#include <sel4platsupport/bootinfo.h>

/* constants */
#define EP_BADGE 0x61 // arbitrary (but unique) number for a badge
#define MSG_DATA 0x6161 // arbitrary data to send

#define APP_PRIORITY seL4_MaxPrio
#define APP_IMAGE_NAME "app"

/* global environment variables */
seL4_BootInfo *info;
simple_t simple;
vka_t vka;
allocman_t *allocman;
vspace_t vspace;

/* static memory for the allocator to bootstrap with */
#define ALLOCATOR_STATIC_POOL_SIZE (BIT(seL4_PageBits) * 10)
UNUSED static char allocator_mem_pool[ALLOCATOR_STATIC_POOL_SIZE];

/* dimensions of virtual memory for the allocator to use */
#define ALLOCATOR_VIRTUAL_POOL_SIZE (BIT(seL4_PageBits) * 100)

/* static memory for virtual memory bootstrapping */
UNUSED static sel4utils_alloc_data_t data;

/* stack for the new thread */
#define THREAD_2_STACK_SIZE 4096
UNUSED static int thread_2_stack[THREAD_2_STACK_SIZE];

int main(void) {
    UNUSED int error = 0;

    /* get boot info */
    info = platsupport_get_bootinfo();
    ZF_LOGF_IF(info == NULL, "Failed to get bootinfo.");

    /* Set up logging and give us a name: useful for debugging if the thread faults */
    zf_log_set_tag_prefix("dynamic-3:");
    NAME_THREAD(seL4_CapInitThreadTCB, "dynamic-3");

    /* init simple */
    simple_default_init_bootinfo(&simple, info);

    /* print out bootinfo and other info about simple */
    simple_print(&simple);

    /* create an allocator */
    allocman = bootstrap_use_current_simple(&simple, ALLOCATOR_STATIC_POOL_SIZE,
                                            allocator_mem_pool);
    ZF_LOGF_IF(allocman == NULL, "Failed to initialize allocator.\n"
               "\tMemory pool sufficiently sized?\n"
               "\tMemory pool pointer valid?\n");

    /* create a vka (interface for interacting with the underlying allocator) */
    allocman_make_vka(&vka, allocman);

    /*? task_1_desc ?*/
    /*? include_task_type_append([("task-1")]) ?*/
    ZF_LOGF_IFERR(error, "Failed to prepare root thread's VSpace for use.\n"
                  "\tsel4utils_bootstrap_vspace_with_bootinfo reserves important vaddresses.\n"
                  "\tIts failure means we can't safely use our vaddrspace.\n");

    /* fill the allocator with virtual memory */
    void *vaddr;
    UNUSED reservation_t virtual_reservation;
    virtual_reservation = vspace_reserve_range(&vspace,
                                               ALLOCATOR_VIRTUAL_POOL_SIZE, seL4_AllRights, 1, &vaddr);
    ZF_LOGF_IF(virtual_reservation.res == NULL, "Failed to reserve a chunk of memory.\n");
    bootstrap_configure_virtual_pool(allocman, vaddr,
                                     ALLOCATOR_VIRTUAL_POOL_SIZE, simple_get_pd(&simple));

    /*? task_2_desc ?*/
    sel4utils_process_t new_process;
    /*? include_task_type_append([("task-2")]) ?*/
    ZF_LOGF_IFERR(error, "Failed to spawn a new thread.\n"
                  "\tsel4utils_configure_process expands an ELF file into our VSpace.\n"
                  "\tBe sure you've properly configured a VSpace manager using sel4utils_bootstrap_vspace_with_bootinfo.\n"
                  "\tBe sure you've passed the correct component name for the new thread!\n");

    /* give the new process's thread a name */
    NAME_THREAD(new_process.thread.tcb.cptr, "dynamic-3: process_2");

    /* create an endpoint */
    vka_object_t ep_object = {0};
    error = vka_alloc_endpoint(&vka, &ep_object);
    ZF_LOGF_IFERR(error, "Failed to allocate new endpoint object.\n");

    /*
     * make a badged endpoint in the new process's cspace.  This copy
     * will be used to send an IPC to the original cap
     */

    /*? task_3_desc ?*/
    /*? include_task_type_append([("task-3")]) ?*/

    /*? task_4_desc ?*/
    /*? include_task_type_append([("task-4")]) ?*/

    ZF_LOGF_IF(new_ep_cap == 0, "Failed to mint a badged copy of the IPC endpoint into the new thread's CSpace.\n"
               "\tsel4utils_mint_cap_to_process takes a cspacepath_t: double check what you passed.\n");

    printf("NEW CAP SLOT: %" PRIxPTR ".\n", ep_cap_path.capPtr);


    /*? task_5_desc ?*/
    /*? include_task_type_append([("task-5")]) ?*/
    ZF_LOGF_IFERR(error, "Failed to spawn and start the new thread.\n"
                  "\tVerify: the new thread is being executed in the root thread's VSpace.\n"
                  "\tIn this case, the CSpaces are different, but the VSpaces are the same.\n"
                  "\tDouble check your vspace_t argument.\n");

    /* we are done, say hello */
    printf("main: hello world\n");

    /*
     * now wait for a message from the new process, then send a reply back
     */

    seL4_Word sender_badge = 0;
    seL4_MessageInfo_t tag = seL4_MessageInfo_new(0, 0, 0, 0);
    seL4_Word msg;

    /*? task_6_desc ?*/
    /*? include_task_type_append([("task-6")]) ?*/
   /* make sure it is what we expected */
    ZF_LOGF_IF(sender_badge != EP_BADGE,
               "The badge we received from the new thread didn't match our expectation.\n");

    ZF_LOGF_IF(seL4_MessageInfo_get_length(tag) != 1,
               "Response data from the new process was not the length expected.\n"
               "\tHow many registers did you set with seL4_SetMR within the new process?\n");


    /* get the message stored in the first message register */
    msg = seL4_GetMR(0);
    printf("main: got a message %#" PRIxPTR " from %#" PRIxPTR "\n", msg, sender_badge);

    /* modify the message */
    seL4_SetMR(0, ~msg);

    /*? task_7_desc ?*/
    /*? include_task_type_append([("task-7")]) ?*/


    return 0;
}
/*-- endfilter -*/
/*-- filter File("app.c") -*/
/*
 * Copyright 2017, Data61, CSIRO (ABN 41 687 119 230).
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

/*
 * seL4 tutorial part 4: application to be run in a process
 */

#include <stdio.h>
#include <assert.h>

#include <sel4/sel4.h>
#include <sel4utils/process.h>

#include <utils/zf_log.h>
#include <sel4utils/sel4_zf_logif.h>

/* constants */
#define MSG_DATA 0x6161 //  arbitrary data to send

int main(int argc, char **argv) {
    seL4_MessageInfo_t tag;
    seL4_Word msg;

    printf("process_2: hey hey hey\n");

    /*
     * send a message to our parent, and wait for a reply
     */

    /* set the data to send. We send it in the first message register */
    tag = seL4_MessageInfo_new(0, 0, 0, 1);
    seL4_SetMR(0, MSG_DATA);

    /*? task_8_desc ?*/
    /*? include_task_type_append([("task-8")]) ?*/


    /* check that we got the expected reply */
    ZF_LOGF_IF(seL4_MessageInfo_get_length(tag) != 1,
               "Length of the data send from root thread was not what was expected.\n"
               "\tHow many registers did you set with seL4_SetMR, within the root thread?\n");

    msg = seL4_GetMR(0);
    ZF_LOGF_IF(msg != ~MSG_DATA,
               "Unexpected response from root thread.\n");

    printf("process_2: got a reply: %#" PRIxPTR "\n", msg);

    return 0;
}
/*-- endfilter -*/
```
/*-- endfilter -*/
