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
'task-8',
'task-9',
'task-10',
'task-11',
'task-12',
'task-13',
'task-14',
'task-15',
]) ?*/

# seL4 Dynamic Libraries: IPC

The tutorial is designed to
teach the basics of seL4 IPC using Endpoint objects, and userspace
paging management. You'll be led through the process of creating a new
thread (retyping an untyped object into a TCB object), and also made to
manually do your own memory management to allocate some virtual memory
for use as the shared memory buffer between your two threads.

Don't gloss over the globals declared before `main()` -- they're declared
for your benefit so you can grasp some of the basic data structures.

You'll observe that the things you've already covered in the second
tutorial are filled out and you don't have to repeat them: in much the
same way, we won't be repeating conceptual explanations on this page, if
they were covered by a previous tutorial in the series.

## Learning outcomes

- Repeat the spawning of a thread. "''If it's nice, do it twice''"
        -- Caribbean folk-saying. Once again, the new thread will be
        sharing its creator's VSpace and CSpace.
- Introduction to the idea of badges, and minting badged copies of
        an Endpoint capability. NB: you don't mint a new copy of the
        Endpoint object, but a copy of the capability that
        references it.
- Basic IPC: sending and receiving: how to make two
        threads communicate.
- IPC Message format, and Message Registers. seL4 binds some of
        the first Message Registers to hardware registers, and the rest
        are transferred in the IPC buffer.
- Understand that each thread has only one IPC buffer, which is
        pointed to by its TCB. It's possible to make a thread wait on
        both an Endpoint and a Notification using "Bound Notifications".
- Understand CSpace pointers, which are really just integers with
        multiple indexes concatenated into one. Understanding them well
        however, is important to understanding how capabilities work. Be
        sure you understand the diagram on the "**CSpace example and
        addressing**" slide.

## Initialising

/*? macros.tutorial_init("dynamic-2") ?*/


## Prerequisites

1. [Set up your machine](https://docs.sel4.systems/HostDependencies).
1. [dynamic-1](https://docs.sel4.systems/Tutorials/dynamic-1)

## Exercises

When you first run this tutorial, you will see a fault as follows:
```
/*-- filter TaskCompletion("task-1", TaskContentType.ALL) -*/
Booting all finished, dropped to user space
Caught cap fault in send phase at address (nil)
while trying to handle:
/*-- endfilter -*/
vm fault on data at address 0x70003c8 with status 0x6
in thread 0xffffff801ffb5400 "rootserver" at address 0x402977
With stack:
0x43df70: 0x0
0x43df78: 0x0
```

### Allocate an IPC buffer

As we mentioned in passing before, threads in seL4 do their own memory
management. You implement your own Virtual Memory Manager, essentially.
To the extent that you must allocate a physical memory frame, then map
it into your thread's page directory -- and even manually allocate page
tables on your own, where necessary.

Here's the first step in a conventional memory manager's process of
allocating memory: first allocate a physical frame. As you would expect,
you cannot directly write to or read from this frame object since it is
not mapped into any virtual address space as yet. Standard restrictions
of a MMU-utilizing kernel apply.

- <https://github.com/seL4/seL4_libs/blob/master/libsel4vka/include/vka/object.h>

```
/*-- set task_1_desc -*/
    /* TASK 1: get a frame cap for the ipc buffer */
    /* hint: vka_alloc_frame()
     * int vka_alloc_frame(vka_t *vka, uint32_t size_bits, vka_object_t *result)
     * @param vka Pointer to vka interface.
     * @param size_bits Frame size: 2^size_bits
     * @param result Structure for the Frame object.  This gets initialised.
     * @return 0 on success
     */
    vka_object_t ipc_frame_object;
/*-- endset -*/
/*? task_1_desc ?*/
/*-- filter ExcludeDocs() -*/
/*-- filter TaskContent("task-1", TaskContentType.COMPLETED) -*/
    error = vka_alloc_frame(&vka, IPCBUF_FRAME_SIZE_BITS, &ipc_frame_object);
    ZF_LOGF_IFERR(error, "Failed to alloc a frame for the IPC buffer.\n"
                  "\tThe frame size is not the number of bytes, but an exponent.\n"
                  "\tNB: This frame is not an immediately usable, virtually mapped page.\n")
/*-- endfilter -*/
/*--filter TaskCompletion("task-1", TaskContentType.ALL) -*/
Booting all finished, dropped to user space
/*-- endfilter -*/
/*-- endfilter -*/
```

On completion, the output will not change.

### Try to map a page

Take note of the line of code that precedes this: the one where a
virtual address is randomly chosen for use. This is because, as we
explained before, the process is responsible for its own Virtual Memory
Management. As such, if it chooses, it can map any page in its VSpace to
physical frame. It can technically choose to do unconventional things,
like not unmap PFN #0. The control of how the address space is managed
is up to the threads that have write-capabilities to that address space.
There is both flexibility and responsibility implied here. Granted, seL4
itself provides strong guarantees of isolation even if a thread decides
to go rogue.

Attempt to map the frame you allocated earlier, into your VSpace. A keen
reader will pick up on the fact that it's unlikely that this will work,
since you'd need a new page table to contain the new page-table-entry.
The tutorial deliberately walks you through both the mapping of a frame
into a VSpace, and the mapping of a new page-table into a VSpace.

- <https://github.com/seL4/seL4_libs/blob/master/libsel4vspace/arch_include/x86/vspace/arch/page.h>
- <https://github.com/seL4/seL4/blob/master/libsel4/arch_include/x86/interfaces/sel4arch.xml>

```
/*-- set task_2_desc -*/
    /* TASK 2: try to map the frame the first time  */
    /* hint 1: seL4_ARCH_Page_Map()
     * The *ARCH* versions of seL4 sys calls are abstractions over the architecture provided by libsel4utils
     * this one is defined as:
     * #define seL4_ARCH_Page_Map seL4_X86_Page_Map
     * The signature for the underlying function is:
     * int seL4_X86_Page_Map(seL4_X86_Page service, seL4_X86_PageDirectory pd, seL4_Word vaddr, seL4_CapRights rights, seL4_X86_VMAttributes attr)
     * @param service Capability to the page to map.
     * @param pd Capability to the VSpace which will contain the mapping.
     * @param vaddr Virtual address to map the page into.
     * @param rights Rights for the mapping.
     * @param attr VM Attributes for the mapping.
     * @return 0 on success.
     *
     * Note: this function is generated during build.  It is generated from the following definition:
     *
     * hint 2: for the rights, use seL4_AllRights
     * hint 3: for VM attributes use seL4_ARCH_Default_VMAttributes
     * Hint 4: It is normal for this function call to fail. That means there are
     *    no page tables with free slots -- proceed to the next step where you'll
     *    be led to allocate a new empty page table and map it into the VSpace,
     *    before trying again.
     */
/*-- endset -*/
/*? task_2_desc ?*/
/*-- filter ExcludeDocs() -*/
/*-- filter TaskContent("task-2", TaskContentType.COMPLETED) -*/
    error = seL4_ARCH_Page_Map(ipc_frame_object.cptr, pd_cap, ipc_buffer_vaddr,
                               seL4_AllRights, seL4_ARCH_Default_VMAttributes);
/*-- endfilter -*/
/*-- endfilter -*/
```

On completion, the output will be as follows:
```
dynamic-2: main@main.c:260 [Err seL4_FailedLookup]:
/*--filter TaskCompletion("task-2", TaskContentType.COMPLETED) -*/
	Failed to allocate new page table.
/*-- endfilter -*/
```

### Allocate a page table

So just as you previously had to manually retype a new frame to use for
your IPC buffer, you're also going to have to manually retype a new
page-table object to use as a leaf page-table in your VSpace.

- <https://github.com/seL4/seL4_libs/blob/master/libsel4vka/include/vka/object.h>

```
/*-- set task_3_desc -*/
        /* TASK 3: create a page table */
        /* hint: vka_alloc_page_table()
         * int vka_alloc_page_table(vka_t *vka, vka_object_t *result)
         * @param vka Pointer to vka interface.
         * @param result Structure for the PageTable object.  This gets initialised.
         * @return 0 on success
         */
/*-- endset -*/
/*? task_3_desc ?*/
/*-- filter ExcludeDocs() -*/
/*-- filter TaskContent("task-3", TaskContentType.COMPLETED) -*/
        vka_object_t pt_object;
        error =  vka_alloc_page_table(&vka, &pt_object);
/*-- endfilter -*/
/*--filter TaskCompletion("task-3", TaskContentType.ALL) -*/
Booting all finished, dropped to user space
/*-- endfilter -*/
/*-- endfilter -*/
 ```
On completion, you will see another fault.

### Map a page table

If you successfully retyped a new page table from an untyped memory
object, you can now map that new page table into your VSpace, and then
try again to finally map the IPC-buffer's frame object into the VSpace.

- <https://github.com/seL4/seL4_libs/blob/master/libsel4vspace/arch_include/x86/vspace/arch/page.h>
- <https://github.com/seL4/seL4/blob/master/libsel4/arch_include/x86/interfaces/sel4arch.xml>

```
/*-- set task_4_desc -*/
        /* TASK 4: map the page table */
        /* hint 1: seL4_ARCH_PageTable_Map()
         * The *ARCH* versions of seL4 sys calls are abstractions over the architecture provided by libsel4utils
         * this one is defined as:
         * #define seL4_ARCH_PageTable_Map seL4_X86_PageTable_Map
         * The signature for the underlying function is:
         * int seL4_X86_PageTable_Map(seL4_X86_PageTable service, seL4_X86_PageDirectory pd, seL4_Word vaddr, seL4_X86_VMAttributes attr)
         * @param service Capability to the page table to map.
         * @param pd Capability to the VSpace which will contain the mapping.
         * @param vaddr Virtual address to map the page table into.
         * @param rights Rights for the mapping.
         * @param attr VM Attributes for the mapping.
         * @return 0 on success.
         *
         * Note: this function is generated during build.  It is generated from the following definition:
         *
         * hint 2: for VM attributes use seL4_ARCH_Default_VMAttributes
         */
/*-- endset -*/
/*? task_4_desc ?*/
/*-- filter ExcludeDocs() -*/
/*-- filter TaskContent("task-4", TaskContentType.COMPLETED) -*/
        error = seL4_ARCH_PageTable_Map(pt_object.cptr, pd_cap,
                                        ipc_buffer_vaddr, seL4_ARCH_Default_VMAttributes);
/*-- endfilter -*/
/*--filter TaskCompletion("task-4", TaskContentType.ALL)--*/
Booting all finished, dropped to user space
/*-- endfilter -*/
/*-- endfilter -*/
```
On completion, you will see another fault.

### Map a page

Use `seL4_ARCH_Page_Map` to map the frame in.
If everything was done correctly, there is no reason why this step
should fail. Complete it and proceed.
```
/*-- set task_5_desc -*/
        /* TASK 5: then map the frame in */
        /* hint 1: use seL4_ARCH_Page_Map() as above
         * hint 2: for the rights, use seL4_AllRights
         * hint 3: for VM attributes use seL4_ARCH_Default_VMAttributes
         */
/*-- endset -*/
/*? task_5_desc ?*/
/*-- filter ExcludeDocs() -*/
/*-- filter TaskContent("task-5", TaskContentType.COMPLETED) -*/
        error = seL4_ARCH_Page_Map(ipc_frame_object.cptr, pd_cap,
                                   ipc_buffer_vaddr, seL4_AllRights, seL4_ARCH_Default_VMAttributes);
/*-- endfilter -*/
/*-- endfilter -*/
```
On completion, you will see the following:
```
/*--filter TaskCompletion("task-5", TaskContentType.COMPLETED) -*/
main: hello world
/*-- endfilter -*/
dynamic-2: main@main.c:464 [Cond failed: seL4_MessageInfo_get_length(tag) != 1]
	Response data from thread_2 was not the length expected.
	How many registers did you set with seL4_SetMR within thread_2?
```

### Allocate an endpoint

Now we have a (fully mapped) IPC buffer -- but no Endpoint object to
send our IPC data across. We must retype an untyped object into a kernel
Endpoint object, and then proceed. This could be done via a more
low-level approach using `seL4_Untyped_Retype()`, but instead, the
tutorial makes use of the VKA allocator. Remember that the VKA allocator
is an seL4 type-aware object allocator? So we can simply ask it for a
new object of a particular type, and it will do the low-level retyping
for us, and return a capability to a new object as requested.

In this case, we want a new Endpoint so we can do IPC. Complete the step
and proceed.

- <https://github.com/seL4/seL4_libs/blob/master/libsel4vka/include/vka/object.h>

```
/*-- set task_6_desc -*/
    /* TASK 6: create an endpoint */
    /* hint: vka_alloc_endpoint()
     * int vka_alloc_endpoint(vka_t *vka, vka_object_t *result)
     * @param vka Pointer to vka interface.
     * @param result Structure for the Endpoint object.  This gets initialised.
     * @return 0 on success
     */
/*-- endset -*/
/*? task_6_desc ?*/
/*-- filter ExcludeDocs() -*/
/*-- filter TaskContent("task-6", TaskContentType.COMPLETED, completion="main: hello world") -*/
    error = vka_alloc_endpoint(&vka, &ep_object);
/*-- endfilter -*/
/*-- endfilter -*/
```
On completion, the output will not change.

### Badge an endpoint

Badges are used to uniquely identify a message queued on an endpoint as
having come from a particular sender. Recall that in seL4, each thread
has only one IPC buffer. If multiple other threads are sending data to a
listening thread, how can that listening thread distinguish between the
data sent by each of its IPC partners? Each sender must "badge" its
capability to its target's endpoint.

Note the distinction: the badge is not applied to the target endpoint,
but to the sender's **capability** to the target endpoint. This
enables the listening thread to mint off copies of a capability to an
Endpoint to multiple senders. Each sender is responsible for applying a
unique badge value to the capability that the listener gave it so that
the listener can identify it.

In this step, you are badging the endpoint that you will use when
sending data to the thread you will be creating later on. The
`vka_mint_object()` call will return a new, badged copy of the
capability to the endpoint that your new thread will listen on. When you
send data to your new thread, it will receive the badge value with the
data, and know which sender you are. Complete the step and proceed.

- <https://github.com/seL4/seL4_libs/blob/master/libsel4vka/include/vka/object_capops.h>
- <https://github.com/seL4/seL4/blob/master/libsel4/mode_include/32/sel4/shared_types.bf>

```
/*-- set task_7_desc -*/
    /* TASK 7: make a badged copy of it in our cspace. This copy will be used to send
     * an IPC message to the original cap */
    /* hint 1: vka_mint_object()
     * int vka_mint_object(vka_t *vka, vka_object_t *object, cspacepath_t *result, seL4_CapRights rights, seL4_Word badge)
     * @param[in] vka The allocator for the cspace.
     * @param[in] object Target object for cap minting.
     * @param[out] result Allocated cspacepath.
     * @param[in] rights The rights for the minted cap.
     * @param[in] badge The badge for the minted cap.
     * @return 0 on success
     *
     * hint 2: for the rights, use seL4_AllRights
     * hint 3: for the badge use EP_BADGE
     */
/*-- endset -*/
/*? task_7_desc ?*/
/*-- filter ExcludeDocs() -*/
/*-- filter TaskContent("task-7", TaskContentType.COMPLETED, completion="main: hello world") -*/
    error = vka_mint_object(&vka, &ep_object, &ep_cap_path, seL4_AllRights,
                            EP_BADGE);
/*-- endfilter -*/
/*-- endfilter -*/
```
On completion, the output will not change.

### Message registers

Here we get a formal introduction to message registers. At first glance,
you might wonder why the `sel4_SetMR()` calls don't specify a message
buffer, and seem to know which buffer to fill out -- and that would be
correct, because they do. They are operating directly on the sending
thread's IPC buffer. Recall that each thread has only one IPC buffer. Go
back and look at your call to `seL4_TCB_Configure()` in step 7 again:
you set the IPC buffer for the new thread in the last 2 arguments to
this function. Likewise, the thread that created **your** main thread
also set an IPC buffer up for you.

So `seL4_SetMR()` and `seL4_GetMR()` simply write to and read from the IPC
buffer you designated for your thread. `MSG_DATA` is uninteresting -- can
be any value. You'll find the `seL4_MessageInfo_t` type explained in the
manuals. In short, it's a header that is embedded in each message that
specifies, among other things, the number of Message Registers that hold
meaningful data, and the number of capabilities that are going to be
transmitted in the message.

- <https://github.com/seL4/seL4/blob/master/libsel4/mode_include/32/sel4/shared_types.bf>
- <https://github.com/seL4/seL4/blob/master/libsel4/include/sel4/functions.h>

```
/*-- set task_8_desc -*/
    /* TASK 8: set the data to send. We send it in the first message register */
    /* hint 1: seL4_MessageInfo_new()
     * seL4_MessageInfo_t CONST seL4_MessageInfo_new(seL4_Uint32 label, seL4_Uint32 capsUnwrapped, seL4_Uint32 extraCaps, seL4_Uint32 length)
     * @param label The value of the label field
     * @param capsUnwrapped The value of the capsUnwrapped field
     * @param extraCaps The value of the extraCaps field
     * @param length The number of message registers to send
     * @return The seL4_MessageInfo_t containing the given values.
     *
     * seL4_MessageInfo_new() is generated during build. It can be found in:
     * build/x86/pc99/libsel4/include/sel4/types_gen.h
     *
     * hint 2: use 0 for the first 3 fields.
     * hint 3: send only 1 message register of data
     *
     * hint 4: seL4_SetMR()
     * void seL4_SetMR(int i, seL4_Word mr)
     * @param i The message register to write
     * @param mr The value of the message register
     *
     * hint 5: send MSG_DATA
     */
/*-- endset -*/
/*? task_8_desc ?*/
/*-- filter ExcludeDocs() -*/
/*-- filter TaskContent("task-8", TaskContentType.COMPLETED) -*/
    tag = seL4_MessageInfo_new(0, 0, 0, 1);
    seL4_SetMR(0, MSG_DATA);
/*-- endfilter -*/
/*-- endfilter -*/
```

On completion, the output should change as follows:
```
dynamic-2: main@main.c:472 [Cond failed: msg != ~MSG_DATA]
/*-- filter TaskCompletion("task-8", TaskContentType.COMPLETED) -*/
	Response data from thread_2's content was not what was expected.
/*-- endfilter -*/
```

### IPC

Now that you've constructed your message and badged the endpoint that
you'll use to send it, it's time to send it. The `seL4_Call()` syscall
will send a message across an endpoint synchronously. If there is no
thread waiting at the other end of the target endpoint, the sender will
block until there is a waiter. The reason for this is because the seL4
kernel would prefer not to buffer IPC data in the kernel address space,
so it just sleeps the sender until a receiver is ready, and then
directly copies the data. It simplifies the IPC logic. There are also
polling send operations, as well as polling receive operations in case
you don't want to be forced to block if there is no receiver on the
other end of an IPC Endpoint.

When you send your badged data using `seL4_Call()`, our receiving thread
(which we created earlier) will pick up the data, see the badge, and
know that it was us who sent the data. Notice how the sending thread
uses the **badged** capability to the endpoint object, and the
receiving thread uses the unmodified original capability to the same
endpoint? The sender must identify itself.

Notice also that the fact that both the sender and the receiver share
the same root CSpace, enables the receiving thread to just casually use
the original, unbadged capability without any extra work needed to make
it accessible.

Notice however also, that while the sending thread has a capability that
grants it full rights to send data across the endpoint since it was the
one that created that capability, the receiver's capability may not
necessarily grant it sending powers (write capability) to the endpoint.
It's entirely possible that the receiver may not be able to send a
response message, if the sender doesn't want it to.

- <https://github.com/seL4/seL4/blob/master/libsel4/sel4_arch_include/ia32/sel4/sel4_arch/syscalls.h>
- <https://github.com/seL4/seL4/blob/master/libsel4/mode_include/32/sel4/shared_types.bf>

```
/*-- set task_9_desc -*/
    /* TASK 9: send and wait for a reply. */
    /* hint: seL4_Call()
     * seL4_MessageInfo_t seL4_Call(seL4_CPtr dest, seL4_MessageInfo_t msgInfo)
     * @param dest The capability to be invoked.
     * @param msgInfo The messageinfo structure for the IPC.  This specifies information about the message to send (such as the number of message registers to send).
     * @return A seL4_MessageInfo_t structure.  This is information about the repy message.
     *
     * hint 2: seL4_MessageInfo_t is generated during build.
     * The type definition and generated field access functions are defined in a generated file:
     * build/x86/pc99/libsel4/include/sel4/types_gen.h
     * It is generated from the following definition:
     */
/*-- endset -*/
/*? task_9_desc ?*/
/*-- filter ExcludeDocs() -*/
/*-- filter TaskContent("task-9", TaskContentType.COMPLETED) -*/
    tag = seL4_Call(ep_cap_path.capPtr, tag);
/*-- endfilter -*/
/*-- endfilter -*/
```

On completion, you should see thread_2 fault as follows:
```
/*--filter TaskCompletion("task-9", TaskContentType.COMPLETED) -*/
thread_2: hallo wereld
thread_2: got a message 0 from 0
/*-- endfilter -*/
Caught cap fault in send phase at address (nil)
while trying to handle:
vm fault on data at address (nil) with status 0x4
in thread 0xffffff801ffb4400 "child of: 'rootserver'" at address (nil)
in thread 0xffffff801ffb4400 "child of: 'rootserver'" at address (nil)
With stack:
```

### Receive a reply

While this task is out of order, since we haven't yet examined the
receive-side of the operation here, it's fairly simple anyway: this task
occurs after the receiver has sent a reply, and it shows the sender now
reading the reply from the receiver. As mentioned before, the
`seL4_GetMR()` calls are simply reading from the calling thread's
designated, single IPC buffer.

- <https://github.com/seL4/seL4/blob/master/libsel4/include/sel4/functions.h>

```
/*-- set task_10_desc -*/
    /* TASK 10: get the reply message */
    /* hint: seL4_GetMR()
     * seL4_Word seL4_GetMR(int i)
     * @param i The message register to retreive
     * @return The message register value
     */
/*-- endset -*/
/*?  task_10_desc ?*/
/*-- filter ExcludeDocs() -*/
/*-- filter TaskContent("task-10", TaskContentType.COMPLETED, completion="thread_2: hallo wereld") -*/
    msg = seL4_GetMR(0);
/*-- endfilter -*/
/*-- endfilter -*/
```
On completion, the output should not change.

### Receive an IPC

We're now in the receiving thread. The `seL4_Recv()` syscall performs a
blocking listen on an Endpoint or Notification capability. When new data
is queued (or when the Notification is signalled), the `seL4_Recv`
operation will unqueue the data and resume execution.

Notice how the `seL4_Recv()` operation explicitly makes allowance for
reading the badge value on the incoming message? The receiver is
explicitly interested in distinguishing the sender.

- <https://github.com/seL4/seL4/blob/master/libsel4/sel4_arch_include/aarch32/sel4/sel4_arch/syscalls.h>
- <https://github.com/seL4/seL4/blob/master/libsel4/mode_include/32/sel4/shared_types.bf>

```
/*-- set task_11_desc -*/
    /* TASK 11: wait for a message to come in over the endpoint */
    /* hint 1: seL4_Recv()
     * seL4_MessageInfo_t seL4_Recv(seL4_CPtr src, seL4_Word* sender)
     * @param src The capability to be invoked.
     * @param sender The badge of the endpoint capability that was invoked by the sender is written to this address.
     * @return A seL4_MessageInfo_t structure
     *
     * hint 2: seL4_MessageInfo_t is generated during build.
     * The type definition and generated field access functions are defined in a generated file:
     * build/x86/pc99/libsel4/include/sel4/types_gen.h
     */
/*-- endset -*/
/*? task_11_desc ?*/
/*-- filter ExcludeDocs() -*/
/*-- filter TaskContent("task-11", TaskContentType.COMPLETED) -*/
    tag = seL4_Recv(ep_object.cptr, &sender_badge);
/*-- endfilter -*/
/*-- endfilter -*/
```
On completion, the output should change slightly:
```
/*-- filter TaskCompletion("task-11", TaskContentType.COMPLETED) -*/
thread_2: got a message 0 from 0x61
/*-- endfilter -*/
```

### Validate the message

These two calls here are just verification of the fidelity of the
transmitted message. It's very unlikely you'll encounter an error here.
Complete them and proceed to the next step.

- <https://github.com/seL4/seL4/blob/master/libsel4/mode_include/32/sel4/shared_types.bf>

```
/*-- set task_12_desc -*/
    /* TASK 12: make sure it is what we expected */
    /* hint 1: check the badge. is it EP_BADGE?
     * hint 2: we are expecting only 1 message register
     * hint 3: seL4_MessageInfo_get_length()
     * seL4_Uint32 CONST seL4_MessageInfo_get_length(seL4_MessageInfo_t seL4_MessageInfo)
     * @param seL4_MessageInfo the seL4_MessageInfo_t to extract a field from
     * @return the number of message registers delivered
     * seL4_MessageInfo_get_length() is generated during build. It can be found in:
     * build/x86/pc99/libsel4/include/sel4/types_gen.h
     */
/*-- endset -*/
/*? task_12_desc ?*/
/*-- filter ExcludeDocs() -*/
/*-- filter TaskContent("task-12", TaskContentType.COMPLETED, completion="thread_2: hallo wereld") -*/
    ZF_LOGF_IF(sender_badge != EP_BADGE,
               "Badge on the endpoint was not what was expected.\n");

    ZF_LOGF_IF(seL4_MessageInfo_get_length(tag) != 1,
               "Length of the data send from root thread was not what was expected.\n"
               "\tHow many registers did you set with seL4_SetMR, within the root thread?\n");
/*-- endfilter -*/
/*-- endfilter -*/
```

On completion, the output should not change.

### Read the message registers

Again, just reading the data from the Message Registers.

- <https://github.com/seL4/seL4/blob/master/libsel4/include/sel4/functions.h>

```
/*-- set task_13_desc -*/
    /* TASK 13: get the message stored in the first message register */
    /* hint: seL4_GetMR()
     * seL4_Word seL4_GetMR(int i)
     * @param i The message register to retreive
     * @return The message register value
     */
/*-- endset -*/
/*? task_13_desc ?*/
/*-- filter ExcludeDocs() -*/
/*-- filter TaskContent("task-13", TaskContentType.COMPLETED, completion="main: hello world") -*/
    msg = seL4_GetMR(0);
/*-- endfilter -*/
/*-- endfilter -*/
```
On completion, the output should change slightly:
```
/*--filter TaskCompletion("task-13", TaskContentType.COMPLETED) -*/
thread_2: got a message 0x6161 from 0x61
/*-- endfilter -*/
```

### Write the message registers

And writing Message Registers again.

- <https://github.com/seL4/seL4/blob/master/libsel4/include/sel4/functions.h>

```
/*-- set task_14_desc -*/
    /* TASK 14: copy the modified message back into the message register */
    /* hint: seL4_SetMR()
     * void seL4_SetMR(int i, seL4_Word mr)
     * @param i The message register to write
     * @param mr The value of the message register
     */
/*-- endset -*/
/*? task_14_desc ?*/
/*-- filter ExcludeDocs() -*/
/*-- filter TaskContent("task-14", TaskContentType.COMPLETED, completion="main: hello world") -*/
    seL4_SetMR(0, msg);
/*-- endfilter -*/
/*-- endfilter -*/
```
On completion, the output should not change.

### Reply to a message

This is a formal introduction to the `Reply` capability which is
automatically generated by the seL4 kernel, whenever an IPC message is
sent using the `seL4_Call()` syscall. This is unique to the `seL4_Call()`
syscall, and if you send data instead with the `seL4_Send()` syscall, the
seL4 kernel will not generate a Reply capability.

The Reply capability solves the issue of a receiver getting a message
from a sender, but not having a sufficiently permissive capability to
respond to that sender. The "Reply" capability is a one-time capability
to respond to a particular sender. If a sender doesn't want to grant the
target the ability to send to it repeatedly, but would like to allow the
receiver to respond to a specific message once, it can use `seL4_Call()`,
and the seL4 kernel will facilitate this one-time permissive response.
Complete the step and pat yourself on the back.

- <https://github.com/seL4/seL4/blob/master/libsel4/sel4_arch_include/ia32/sel4/sel4_arch/syscalls.h>
- <https://github.com/seL4/seL4/blob/master/libsel4/mode_include/32/sel4/shared_types.bf>

```
/*-- set task_15_desc -*/
    /* TASK 15: send the message back */
    /* hint 1: seL4_ReplyRecv()
     * seL4_MessageInfo_t seL4_ReplyRecv(seL4_CPtr dest, seL4_MessageInfo_t msgInfo, seL4_Word *sender)
     * @param dest The capability to be invoked.
     * @param msgInfo The messageinfo structure for the IPC.  This specifies information about the message to send (such as the number of message registers to send) as the Reply part.
     * @param sender The badge of the endpoint capability that was invoked by the sender is written to this address.  This is a result of the Wait part.
     * @return A seL4_MessageInfo_t structure.  This is a result of the Wait part.
     *
     * hint 2: seL4_MessageInfo_t is generated during build.
     * The type definition and generated field access functions are defined in a generated file:
     * build/x86/pc99/libsel4/include/sel4/types_gen.h
     * It is generated from the following definition:
     */
/*-- endset -*/
/*? task_15_desc ?*/
/*-- filter ExcludeDocs() -*/
/*-- filter TaskContent("task-15", TaskContentType.COMPLETED, completion="main: hello world") -*/
    seL4_ReplyRecv(ep_object.cptr, tag, &sender_badge);
/*-- endfilter -*/
/*-- endfilter -*/
```
On completion, the output should change, with the fault message replaced with the following:
```
/*--filter TaskCompletion("task-15", TaskContentType.COMPLETED) -*/
main: got a reply: [0xffff9e9e|0xffffffffffff9e9e]
/*-- endfilter -*/
```
That's it for this tutorial.

/*? macros.help_block() ?*/

/*- filter ExcludeDocs() -*/
/*? ExternalFile("CMakeLists.txt") ?*/
```
/*-- filter File("main.c") -*/
/*
 * Copyright 2017, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

/*
 * seL4 tutorial part 3: IPC between 2 threads
 */

/* Include Kconfig variables. */
#include <autoconf.h>

#include <stdio.h>
#include <assert.h>

#include <sel4/sel4.h>
#include <sel4runtime.h>
#include <sel4runtime/gen_config.h>

#include <simple/simple.h>
#include <simple-default/simple-default.h>

#include <vka/object.h>
#include <vka/object_capops.h>

#include <allocman/allocman.h>
#include <allocman/bootstrap.h>
#include <allocman/vka.h>

#include <vspace/vspace.h>

#include <sel4utils/vspace.h>
#include <sel4utils/mapping.h>

#include <utils/arith.h>
#include <utils/zf_log.h>
#include <sel4utils/sel4_zf_logif.h>

#include <sel4platsupport/bootinfo.h>

/* constants */
#define IPCBUF_FRAME_SIZE_BITS 12 // use a 4K frame for the IPC buffer
#define IPCBUF_VADDR 0x7000000 // arbitrary (but free) address for IPC buffer

#define EP_BADGE 0x61 // arbitrary (but unique) number for a badge
#define MSG_DATA 0x6161 // arbitrary data to send

/* global environment variables */
seL4_BootInfo *info;
simple_t simple;
vka_t vka;
allocman_t *allocman;

/* variables shared with second thread */
vka_object_t ep_object;
cspacepath_t ep_cap_path;

/* static memory for the allocator to bootstrap with */
#define ALLOCATOR_STATIC_POOL_SIZE (BIT(seL4_PageBits) * 10)
UNUSED static char allocator_mem_pool[ALLOCATOR_STATIC_POOL_SIZE];

/* stack for the new thread */
#define THREAD_2_STACK_SIZE 512
static uint64_t thread_2_stack[THREAD_2_STACK_SIZE];

/* tls region for the new thread */
static char tls_region[CONFIG_SEL4RUNTIME_STATIC_TLS] = {};

/* convenience function */
extern void name_thread(seL4_CPtr tcb, char *name);

/* function to run in the new thread */
void thread_2(void) {
    seL4_Word sender_badge = 0;
    UNUSED seL4_MessageInfo_t tag;
    seL4_Word msg = 0;

    printf("thread_2: hallo wereld\n");

/*? task_11_desc ?*/
/*? include_task_type_append([("task-11")]) ?*/
/*? task_12_desc ?*/
/*? include_task_type_append([("task-12")]) ?*/
/*? task_13_desc ?*/
/*? include_task_type_append([("task-13")]) ?*/
    printf("thread_2: got a message %#" PRIxPTR " from %#" PRIxPTR "\n", msg, sender_badge);

    /* modify the message */
    msg = ~msg;
/*? task_14_desc ?*/
/*? include_task_type_append([("task-14")]) ?*/
/*? task_15_desc ?*/
/*? include_task_type_append([("task-15")]) ?*/
}

int main(void) {
    UNUSED int error;

    /* get boot info */
    info = platsupport_get_bootinfo();
    ZF_LOGF_IF(info == NULL, "Failed to get bootinfo.");

    /* Set up logging and give us a name: useful for debugging if the thread faults */
    zf_log_set_tag_prefix("dynamic-2:");
    name_thread(seL4_CapInitThreadTCB, "dynamic-2");

    /* init simple */
    simple_default_init_bootinfo(&simple, info);

    /* print out bootinfo and other info about simple */
    simple_print(&simple);

    /* create an allocator */
    allocman = bootstrap_use_current_simple(&simple, ALLOCATOR_STATIC_POOL_SIZE,        allocator_mem_pool);
    ZF_LOGF_IF(allocman == NULL, "Failed to initialize alloc manager.\n"
               "\tMemory pool sufficiently sized?\n"
               "\tMemory pool pointer valid?\n");

    /* create a vka (interface for interacting with the underlying allocator) */
    allocman_make_vka(&vka, allocman);

    /* get our cspace root cnode */
    seL4_CPtr cspace_cap;
    cspace_cap = simple_get_cnode(&simple);

    /* get our vspace root page directory */
    seL4_CPtr pd_cap;
    pd_cap = simple_get_pd(&simple);

    /* create a new TCB */
    vka_object_t tcb_object = {0};
    error = vka_alloc_tcb(&vka, &tcb_object);
    ZF_LOGF_IFERR(error, "Failed to allocate new TCB.\n"
                  "\tVKA given sufficient bootstrap memory?");

    /*
     * create and map an ipc buffer:
     */
/*? task_1_desc ?*/
/*? include_task_type_append([("task-1")]) ?*/

   /*
     * map the frame into the vspace at ipc_buffer_vaddr.
     * To do this we first try to map it in to the root page directory.
     * If there is already a page table mapped in the appropriate slot in the
     * page diretory where we can insert this frame, then this will succeed.
     * Otherwise we first need to create a page table, and map it in to
     * the page directory, before we can map the frame in. */

    seL4_Word ipc_buffer_vaddr = IPCBUF_VADDR;

/*? task_2_desc ?*/
/*? include_task_type_append([("task-2")]) ?*/

    if (error != 0) {
/*? task_3_desc ?*/
/*? include_task_type_append([("task-3")]) ?*/
       ZF_LOGF_IFERR(error, "Failed to allocate new page table.\n");

/*? task_4_desc ?*/
/*? include_task_type_append([("task-4")]) ?*/
       ZF_LOGF_IFERR(error, "Failed to map page table into VSpace.\n"
                      "\tWe are inserting a new page table into the top-level table.\n"
                      "\tPass a capability to the new page table, and not for example, the IPC buffer frame vaddr.\n")

/*? task_5_desc ?*/
/*? include_task_type_append([("task-5")]) ?*/
        ZF_LOGF_IFERR(error, "Failed again to map the IPC buffer frame into the VSpace.\n"
                      "\t(It's not supposed to fail.)\n"
                      "\tPass a capability to the IPC buffer's physical frame.\n"
                      "\tRevisit the first seL4_ARCH_Page_Map call above and double-check your arguments.\n");
   }

/*? task_6_desc ?*/
/*? include_task_type_append([("task-6")]) ?*/
   ZF_LOGF_IFERR(error, "Failed to allocate new endpoint object.\n");

/*? task_7_desc ?*/
/*? include_task_type_append([("task-7")]) ?*/
    ZF_LOGF_IFERR(error, "Failed to mint new badged copy of IPC endpoint.\n"
                  "\tseL4_Mint is the backend for vka_mint_object.\n"
                  "\tseL4_Mint is simply being used here to create a badged copy of the same IPC endpoint.\n"
                  "\tThink of a badge in this case as an IPC context cookie.\n");

    /* initialise the new TCB */
    error = seL4_TCB_Configure(tcb_object.cptr, seL4_CapNull,
                               cspace_cap, seL4_NilData, pd_cap, seL4_NilData,
                               ipc_buffer_vaddr, ipc_frame_object.cptr);
    ZF_LOGF_IFERR(error, "Failed to configure the new TCB object.\n"
                  "\tWe're running the new thread with the root thread's CSpace.\n"
                  "\tWe're running the new thread in the root thread's VSpace.\n");


    /* give the new thread a name */
    name_thread(tcb_object.cptr, "dynamic-2: thread_2");

    /* set start up registers for the new thread */
    seL4_UserContext regs = {0};
    size_t regs_size = sizeof(seL4_UserContext) / sizeof(seL4_Word);

    /* set instruction pointer where the thread shoud start running */
    sel4utils_set_instruction_pointer(&regs, (seL4_Word)thread_2);

    /* check that stack is aligned correctly */
    const int stack_alignment_requirement = sizeof(seL4_Word) * 2;
    uintptr_t thread_2_stack_top = (uintptr_t)thread_2_stack + sizeof(thread_2_stack);

    ZF_LOGF_IF(thread_2_stack_top % (stack_alignment_requirement) != 0,
               "Stack top isn't aligned correctly to a %dB boundary.\n"
               "\tDouble check to ensure you're not trampling.",
               stack_alignment_requirement);

    /* set stack pointer for the new thread. remember the stack grows down */
    sel4utils_set_stack_pointer(&regs, thread_2_stack_top);

    /* actually write the TCB registers. */
    error = seL4_TCB_WriteRegisters(tcb_object.cptr, 0, 0, regs_size, &regs);
    ZF_LOGF_IFERR(error, "Failed to write the new thread's register set.\n"
                  "\tDid you write the correct number of registers? See arg4.\n");

   /* create a thread local storage (TLS) region for the new thread to store the
      ipc buffer pointer */
    uintptr_t tls = sel4runtime_write_tls_image(tls_region);
    seL4_IPCBuffer *ipcbuf = (seL4_IPCBuffer*)ipc_buffer_vaddr;
    error = sel4runtime_set_tls_variable(tls, __sel4_ipc_buffer, ipcbuf);
    ZF_LOGF_IF(error, "Failed to set ipc buffer in TLS of new thread");
    /* set the TLS base of the new thread */
    error = seL4_TCB_SetTLSBase(tcb_object.cptr, tls);
    ZF_LOGF_IF(error, "Failed to set TLS base");

    /* start the new thread running */
    error = seL4_TCB_Resume(tcb_object.cptr);
    ZF_LOGF_IFERR(error, "Failed to start new thread.\n");

    /* we are done, say hello */
    printf("main: hello world\n");

    /*
     * now send a message to the new thread, and wait for a reply
     */

    seL4_Word msg = 0;
    seL4_MessageInfo_t tag = seL4_MessageInfo_new(0, 0, 0, 0);

/*? task_8_desc ?*/
/*? include_task_type_append([("task-8")]) ?*/
/*? task_9_desc ?*/
/*? include_task_type_append([("task-9")]) ?*/
/*? task_10_desc ?*/
/*? include_task_type_append([("task-10")]) ?*/

    /* check that we got the expected repy */
    ZF_LOGF_IF(seL4_MessageInfo_get_length(tag) != 1,
               "Response data from thread_2 was not the length expected.\n"
               "\tHow many registers did you set with seL4_SetMR within thread_2?\n");

    ZF_LOGF_IF(msg != ~MSG_DATA,
               "Response data from thread_2's content was not what was expected.\n");

    printf("main: got a reply: %#" PRIxPTR "\n", msg);

    return 0;
}
/*- endfilter -*/
```
/*- endfilter -*/
