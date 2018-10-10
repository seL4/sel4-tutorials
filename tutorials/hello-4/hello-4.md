---
toc: true
---

# seL4 Tutorial 4
The fourth tutorial is largely trying
to show how a separate ELF file can be loaded and expanded into a
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

## Walkthrough
```
# create a build directory
mkdir build_hello_4
cd build_hello_4
# initialise your build directory
../init --plat pc99 --tut hello-4
# build it
ninja
# run it in qemu
./simulate
```

Look for `TASK` in the `hello-4/src` and `hello-4/hello-4-app`
directory for each task. The first set of tasks are in
`hello-4/src/main.c` and the rest are in `hello-4/hello-4-app/src/main.c`

### TASK 1


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

<https://github.com/seL4/seL4_libs/blob/master/libsel4utils/include/sel4utils/vspace.h>

<https://github.com/seL4/seL4_libs/blob/master/libsel4utils/include/sel4utils/vspace.h>

### TASK 2


`sel4utils_configure_process_custom` took a large amount of the work
out of creating a new "processs". We skipped a number of steps. Take a
look at the source for `sel4utils_configure_process_custom()` and
notice how it spawns the new thread with its own CSpace by
automatically. This will have an effect on our tutorial! It means that
the new thread we're creating will not share a CSpace with our main
thread.

<https://github.com/seL4/seL4_libs/blob/master/libsel4utils/include/sel4utils/process.h>

### TASK 3


This should be a fairly easy step to complete!

### TASK 4


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

<https://github.com/seL4/seL4_libs/blob/master/libsel4vka/include/vka/vka.h>

### TASK 5


As discussed above, we now just mint a badged copy of a capability to
the endpoint we're listening on, into the new thread's CSpace, in the
free slot that the VKA library found for us.

<https://github.com/seL4/seL4_libs/blob/master/libsel4utils/include/sel4utils/process.h>

<https://github.com/seL4/seL4/blob/master/libsel4/include/sel4/types_32.bf>

### TASK 6


So now that we've given the new thread everything it needs to
communicate with us, we can let it run. Complete this step and proceed.

<https://github.com/seL4/seL4_libs/blob/master/libsel4utils/include/sel4utils/process.h>

### TASK 7


We now wait for the new thread to send us data using `seL4_Recv()`...

Then we verify the fidelity of the data that was transmitted.

<https://github.com/seL4/seL4/blob/master/libsel4/sel4_arch_include/ia32/sel4/sel4_arch/syscalls.h>

<https://github.com/seL4/seL4/blob/master/libsel4/include/sel4/shared_types_32.bf>

### TASK 8


Another demonstration of the `sel4_Reply()` facility: we reply to the
message sent by the new thread.

<https://github.com/seL4/seL4/blob/master/libsel4/sel4_arch_include/ia32/sel4/sel4_arch/syscalls.h#L359>

<https://github.com/seL4/seL4/blob/master/libsel4/include/sel4/shared_types_32.bf#L15>

### TASK 9


In the new thread, we initiate communications by using `seL4_Call()`. As
outlined above, the receiving thread replies to us using
`sel4_ReplyRecv()`. The new thread then checks the fidelity of the data
that was sent, and that's the end.

<https://github.com/seL4/seL4/blob/master/libsel4/sel4_arch_include/ia32/sel4/sel4_arch/syscalls.h>