<!--
  Copyright 2017, Data61, CSIRO (ABN 41 687 119 230)

  SPDX-License-Identifier: BSD-2-Clause
-->

/*? declare_task_ordering(['cnode-start', 'cnode-size', 'cnode-copy', 'cnode-delete', 'cnode-invoke']) ?*/


# Capabilities

This tutorial provides a basic introduction to seL4 capabilities.

## Prerequisites

1. [Set up your machine](https://docs.sel4.systems/HostDependencies).
2. [Hello world](https://docs.sel4.systems/Tutorials/hello-world)

## Initialising

/*? macros.tutorial_init("capabilities") ?*/

## Outcomes

By the end of this tutorial, you should be familiar with:

1. The jargon CNode, CSpace, CSlot.
2. Know how to invoke a capability.
3. Know how to delete and copy CSlots.

## Background

### What is a capability?

A *capability* is a unique, unforgeable token that gives the possessor
permission to access an entity or object in system. One way to think of a
capability is as a pointer with access rights. There are three kinds of
capabilities in seL4:

* capabilities that control access to kernel objects such as thread control blocks,
* capabilities that control access to abstract resources such as `IRQControl`, and
* untyped capabilities, that are responsible for memory ranges and allocation
  from those (see also the [Untyped] tutorial).

[untyped]: https://docs.sel4.systems/Tutorials/untyped.html

In seL4, capabilities to all resources controlled by the kernel are given to the root
task on initialisation. To change the state of any resource, user code can use
the kernel API, available in `libsel4` to request an operation on the resource
a specific capability points to.

For example, the root task is provided with a capability to its own thread
control block (TCB), `seL4_CapInitThreadTCB`, a constant defined by `libsel4`.
To change the properties of the initial TCB, one can use any of the [TCB API
methods](https://docs.sel4.systems/projects/sel4/api-doc.html#sel4_tcb) on this
capability.

Below is an example which changes the stack pointer of the root task's TCB, a
common operation in the root task if a larger stack is needed:

```c
    seL4_UserContext registers;
    seL4_Word num_registers = sizeof(seL4_UserContext)/sizeof(seL4_Word);

    /* Read the registers of the TCB that the capability in seL4_CapInitThreadTCB grants access to. */
    seL4_Error error = seL4_TCB_ReadRegisters(seL4_CapInitThreadTCB, 0, 0, num_registers, &registers);
    assert(error == seL4_NoError);

    /* set new register values */
    registers.sp = new_sp; // the new stack pointer, derived by prior code.

    /* Write new values */
    error = seL4_TCB_WriteRegisters(seL4_CapInitThreadTCB, 0, 0, num_registers, &registers);
    assert(error == seL4_NoError);
```

The first argument of `seL4_TCB_ReadRegisters` and `seL4_TCB_WriteRegisters`
addresses the capability in slot `seL4_CapInitThreadTCB`. We will explain
addressing and slots below. The rest of the arguments are specific to the
invocations. Further documentation is available on
[TCB_ReadRegisters](https://docs.sel4.systems/projects/sel4/api-doc.html#read-registers)
and
[TCB_WriteRegisters](https://docs.sel4.systems/projects/sel4/api-doc.html#write-registers).

### CNodes and CSlots

A *CNode* (capability-node) is an object full of capabilities: you can think of a CNode as an array of capabilities.
We refer to slots as *CSlots* (capability-slots). In the example above, `seL4_CapInitThreadTCB` is the slot in the root
task's CNode that contains the capability to the root task's TCB.
Each CSlot in a CNode can be in the following state:

* empty: the CNode slot contains a null capability,
* full: the slot contains a capability to a kernel resource.

By convention the 0th CSlot is kept empty, for the same reasons as keeping NULL unmapped in
 process virtual address spaces: to avoid errors when uninitialised slots are used unintentionally.

The field `info->CNodeSizeBits` gives a measure of the size of the initial
CNode: it will have `1 << CNodeSizeBits` CSlots. A CSlot has
`1 << seL4_SlotBits` bytes, so the size of a CNode in bytes is
`1 << (CNodeSizeBits + seL4_SlotBits`.

### CSpaces

A *CSpace* (capability-space) is the full range of capabilities accessible to a thread, which may be
formed of one or more CNodes. In this tutorial, we focus on the CSpace constructed for the root task
by seL4's initialisation protocol, which consists of one CNode.

### CSpace addressing

To refer to a capability and perform operations on it, you must *address* the
capability. There are two ways to address capabilities in the seL4 API. First is
by invocation, the second is by direct addressing. Invocation is a shorthand and
it is what we used to manipulate the registers of the root task's TCB, which we
now explain in further detail.

#### Invocation

Each thread has a special CNode capability installed in its TCB as its *CSpace
root*. This root can be empty (a null cap), for instance when the thread is not
authorised to invoke any capabilities at all, or it can be a capability to a
CNode. The root task always has a CSpace root capability that points to a CNode.

In an *invocation*, a CSlot is addressed by implicitly invoking the CSpace root
of the thread that is performing the invocation. In the code example above, we
use an invocation on the `seL4_CapInitThreadTCB` CSlot to read from and write to
the registers of the TCB represented by the capability in that specific CSlot.

```c
seL4_TCB_WriteRegisters(seL4_CapInitThreadTCB, 0, 0, num_registers, &registers);
```

This implicitly looks up the `seL4_CapInitThreadTCB` CSlot in the CNode pointed to
by the CSpace root capability of the calling thread, which here is the root task.

When it is clear by context and not otherwise relevant, we sometimes identify
the capability with the object it points to. So, instead of saying "the CNode
pointed to by the CSpace root capability", we sometimes say "the CSpace root".
If not sure, it is better to be precise. Like structs and pointers in C, objects
and capabilities are not actually interchangeable: one object can be pointed to
by multiple capabilities, and each of these capabilities could have a different
level of permissions to that object.


#### Direct CSpace addressing

In contrast to invocation addressing, *direct addressing* allows you to specify
the CNode to look up in, rather than implicitly using the CSpace root. This form
of addressing is primarily used to construct and manipulate the shape of CSpaces
-- potentially the CSpace of another thread. Note that direct addressing also
requires invocation: the operation occurs by invoking a CNode capability, which
itself is indexed from the CSpace root.

The following fields are used when directly addressing CSlots:

* *_service/root* A capability to the CNode to operate on.
* *index* The index of the CSlot in the CNode to address.
* *depth* How far to traverse the CNode before resolving the CSlot.

For the initial, single-level CSpace, the *depth* value is always `seL4_WordBits`. For invocations, the depth is always
 implicitly `seL4_WordBits`.
More on CSpace depth will be discussed in future tutorials.

In the example below, we directly address the root task's TCB to make a copy of
it in the 0th slot in the CSpace root. [CNode
copy](https://docs.sel4.systems/projects/sel4/api-doc.html#copy) requires two
CSlots to be directly addressed: the destination CSlot, and the source CSlot.
`seL4_CapInitThreadCNode` is used in three different roles here: as source root,
as destination root, and as source slot. It is used as source and destination
root, because we are copying within the same CNode -- the CNode of the initial
thread. It is used as source slot, because within the CNode of the initial
thread, `seL4_CapInitThreadCNode` is the slot of the capability we want to copy
(and the slot `0` is the destination).

```c
    seL4_Error error = seL4_CNode_Copy(
        seL4_CapInitThreadCNode, 0, seL4_WordBits, // destination root, slot, and depth
        seL4_CapInitThreadCNode, seL4_CapInitThreadTCB, seL4_WordBits, // source root, slot, and depth
        seL4_AllRights);
    assert(error == seL4_NoError);
```

All [CNode invocations](https://docs.sel4.systems/projects/sel4/api-doc.html#sel4_cnode) require direct CSpace addressing.

### Initial CSpace

The root task has a CSpace, set up by seL4 during boot, which contains capabilities to all
resources manages by seL4. We have already seen several capabilities in the root CSpace: `seL4_CapInitThreadTCB`,
 and `seL4_CapInitThreadCNode`. Both of these are specified by constants in `libsel4`, however not all initial
 capabilities are statically specified. Other capabilities are described by the `seL4_BootInfo` data structure,
 described in `libsel4` and initialised by seL4. `seL4_BootInfo` describes ranges of initial capabilities,
 including free slots available in the initial CSpace.

## Exercises

The initial state of this tutorial provides you with the BootInfo structure,
and calculates the size (in slots) of the initial CNode object.

```c
/*-- filter TaskContent("cnode-start", TaskContentType.ALL, subtask='init') -*/
int main(int argc, char *argv[]) {

    /* parse the location of the seL4_BootInfo data structure from
    the environment variables set up by the default crt0.S */
    seL4_BootInfo *info = platsupport_get_bootinfo();

    size_t initial_cnode_object_size = BIT(info->initThreadCNodeSizeBits);
    printf("Initial CNode is %zu slots in size\n", initial_cnode_object_size);
/*-- endfilter -*/
```

When you run the tutorial without changes, you will see something like the following output:

```
Booting all finished, dropped to user space
Initial CNode is 65536 slots in size
The CNode is 0 bytes in size
<<seL4(CPU 0) [decodeInvocation/530 T0xffffff801ffb5400 "rootserver" @401397]: Attempted to invoke a null cap #4095.>>
main@main.c:33 [Cond failed: error]
    Failed to set priority
```

By the end of the tutorial all of the output will make sense. For now, the first line is from the kernel.
The second is the `printf`, telling you the size of the initial CNode.
The third line stating the number of slots in the CSpace, is incorrect, and your first task is to fix that.

### How big is your CSpace?

**Exercise:** refer to the background above, and calculate the number of bytes occupied by the initial thread's CSpace.

```c
/*-- filter TaskContent("cnode-start", TaskContentType.ALL, subtask='size') -*/
    size_t initial_cnode_object_size_bytes = 0; // TODO calculate this.
    printf("The CNode is %zu bytes in size\n", initial_cnode_object_size_bytes);
/*-- endfilter -*/
```

/*-- filter ExcludeDocs() -*/

```c
/*-- filter TaskContent("cnode-size", TaskContentType.COMPLETED, subtask='size', completion='The CNode is [0-9]+ bytes in size') -*/
    size_t initial_cnode_object_size_bytes = initial_cnode_object_size * (1u << seL4_SlotBits);
    printf("The CNode is %zu bytes in size\n", initial_cnode_object_size_bytes);
/*-- endfilter -*/
```

/*-- endfilter -*/

### Copy a capability between CSlots

After the output showing the number of bytes in the CSpace, you will see an error:

```
<<seL4(CPU 0) [decodeInvocation/530 T0xffffff801ffb5400 "rootserver" @401397]: Attempted to invoke a null cap #4095.>>
main@main.c:33 [Cond failed: error]
    Failed to set priority
```

The error occurs as the existing code tries to set the priority of the initial thread's TCB by
 invoking the last CSlot in the CSpace, which is currently empty. seL4 then returns an error code,
 and our check that the operation succeeded fails.

**Exercise:** fix this problem by making another copy of the TCB capability into the last slot in the CNode.

```c
/*-- filter TaskContent("cnode-start", TaskContentType.ALL, subtask='copy', completion='Failed to set priority') -*/
    seL4_CPtr first_free_slot = info->empty.start;
    seL4_Error error = seL4_CNode_Copy(seL4_CapInitThreadCNode, first_free_slot, seL4_WordBits,
                                       seL4_CapInitThreadCNode, seL4_CapInitThreadTCB, seL4_WordBits,
                                       seL4_AllRights);
    ZF_LOGF_IF(error, "Failed to copy cap!");
    seL4_CPtr last_slot = info->empty.end - 1;
    /* TODO use seL4_CNode_Copy to make another copy of the initial TCB capability to the last slot in the CSpace */

    /* set the priority of the root task */
    error = seL4_TCB_SetPriority(last_slot, last_slot, 10);
    ZF_LOGF_IF(error, "Failed to set priority");
/*-- endfilter -*/
```

/*-- filter ExcludeDocs() -*/

```c
/*-- filter TaskContent("cnode-copy", TaskContentType.COMPLETED, subtask='copy', completion='first_free_slot is not empty') -*/
    seL4_CPtr first_free_slot = info->empty.start;
    seL4_Error error = seL4_CNode_Copy(seL4_CapInitThreadCNode, first_free_slot, seL4_WordBits,
                                       seL4_CapInitThreadCNode, seL4_CapInitThreadTCB, seL4_WordBits,
                                       seL4_AllRights);
    ZF_LOGF_IF(error, "Failed to copy cap!");
    seL4_CPtr last_slot = info->empty.end - 1;

    /* use seL4_CNode_Copy to make another copy of the initial TCB capability to the last slot in the CSpace */
    error = seL4_CNode_Copy(seL4_CapInitThreadCNode, last_slot, seL4_WordBits,
                      seL4_CapInitThreadCNode, first_free_slot, seL4_WordBits, seL4_AllRights);
    ZF_LOGF_IF(error, "Failed to copy cap!");

    /* set the priority of the root task */
    error = seL4_TCB_SetPriority(last_slot, last_slot, 10);
    ZF_LOGF_IF(error, "Failed to set priority");
/*-- endfilter -*/
```

/*-- endfilter -*/
On success, you will now see the output:

```
<<seL4(CPU 0) [decodeCNodeInvocation/94 T0xffffff801ffb5400 "rootserver" @401397]: CNode Copy/Mint/Move/Mutate: Destination not empty.>>
main@main.c:44 [Cond failed: error != seL4_FailedLookup]
    first_free_slot is not empty
```

Which will be fixed in the next exercise.

### How do you delete capabilities?

The provided code checks that both `first_free_slot` and `last_slot` are empty, which of course is not
true, as you copied TCB capabilities into those CSlots. Checking if CSlots are empty is done
by a neat hack: by attempting to move the CSlots onto themselves. This should fail with an error code
`seL4_FailedLookup` if the source CSLot is empty, and an `seL4_DeleteFirst` if not.

**Exercise:** delete both copies of the TCB capability.

* You can either use `seL4_CNode_Delete` on the copies, or
* `seL4_CNode_Revoke` on the original capability to achieve this.


 ```c
/*-- filter TaskContent("cnode-start", TaskContentType.ALL, subtask='delete') -*/
    // TODO delete the created TCB capabilities

    // check first_free_slot is empty
    error = seL4_CNode_Move(seL4_CapInitThreadCNode, first_free_slot, seL4_WordBits,
                            seL4_CapInitThreadCNode, first_free_slot, seL4_WordBits);
    ZF_LOGF_IF(error != seL4_FailedLookup, "first_free_slot is not empty");

    // check last_slot is empty
    error = seL4_CNode_Move(seL4_CapInitThreadCNode, last_slot, seL4_WordBits,
                            seL4_CapInitThreadCNode, last_slot, seL4_WordBits);
    ZF_LOGF_IF(error != seL4_FailedLookup, "last_slot is not empty");
/*-- endfilter -*/
```

/*-- filter ExcludeDocs() -*/

```c
/*-- filter TaskContent("cnode-delete", TaskContentType.COMPLETED, subtask='delete', completion='Failed to suspend current thread') -*/
    // delete the created TCB capabilities
    seL4_CNode_Revoke(seL4_CapInitThreadCNode, seL4_CapInitThreadTCB, seL4_WordBits);

    // check first_free_slot is empty
    error = seL4_CNode_Move(seL4_CapInitThreadCNode, first_free_slot, seL4_WordBits,
                            seL4_CapInitThreadCNode, first_free_slot, seL4_WordBits);
    ZF_LOGF_IF(error != seL4_FailedLookup, "first_free_slot is not empty");

    // check last_slot is empty
    error = seL4_CNode_Move(seL4_CapInitThreadCNode, last_slot, seL4_WordBits,
                            seL4_CapInitThreadCNode, last_slot, seL4_WordBits);
    ZF_LOGF_IF(error != seL4_FailedLookup, "last_slot is not empty");
/*-- endfilter -*/
```

/*-- endfilter -*/

On success, the output will now show:

```
<<seL4(CPU 0) [decodeCNodeInvocation/106 T0xffffff801ffb5400 "rootserver" @401397]: CNode Copy/Mint/Move/Mutate: Source slot invalid or empty.>>
<<seL4(CPU 0) [decodeCNodeInvocation/106 T0xffffff801ffb5400 "rootserver" @401397]: CNode Copy/Mint/Move/Mutate: Source slot invalid or empty.>>
Suspending current thread
main@main.c:56 Failed to suspend current thread
```

#### Invoking capabilities

**Exercise** Use `seL4_TCB_Suspend` to try and suspend the current thread.

```c
/*-- filter TaskContent("cnode-start", TaskContentType.ALL, subtask='invoke') -*/
    printf("Suspending current thread\n");
    // TODO suspend the current thread
    ZF_LOGF("Failed to suspend current thread\n");
/*-- endfilter -*/
```

/*-- filter ExcludeDocs() -*/

```c
/*-- filter TaskContent("cnode-invoke", TaskContentType.COMPLETED, subtask='invoke', completion='Suspending current thread') -*/
    printf("Suspending current thread\n");
    seL4_TCB_Suspend(seL4_CapInitThreadTCB);
    ZF_LOGF("Failed to suspend current thread\n");
/*-- endfilter -*/
```

/*-- endfilter -*/

On success, the output will be as follows:

```
<<seL4(CPU 0) [decodeCNodeInvocation/106 T0xffffff801ffb5400 "rootserver" @401397]: CNode Copy/Mint/Move/Mutate: Source slot invalid or empty.>>
<<seL4(CPU 0) [decodeCNodeInvocation/106 T0xffffff801ffb5400 "rootserver" @401397]: CNode Copy/Mint/Move/Mutate: Source slot invalid or empty.>>
Suspending current thread
```

### Further exercises

That's all for the detailed content of this tutorial. Below we list other ideas for exercises you can try,
to become more familiar with CSpaces.

* Use a data structure to track which CSlots in a CSpace are free.
* Make copies of the entire CSpace described by `seL4_BootInfo`
* Experiment with other [CNode invocations](https://docs.sel4.systems/projects/sel4/api-doc.html#sel4_cnode).

/*? macros.help_block() ?*/
/*-- filter ExcludeDocs() -*/

```c
/*-- filter File("src/main.c") -*/
#include <stdio.h>
#include <sel4/sel4.h>
#include <sel4platsupport/bootinfo.h>
#include <utils/util.h>

/*? include_task_type_append([("cnode-start", 'init')]) ?*/

/*? include_task_type_replace([("cnode-start", 'size'), ("cnode-size", 'size')]) ?*/
/*? include_task_type_replace([("cnode-start", 'copy'), ("cnode-copy", 'copy')]) ?*/
/*? include_task_type_replace([("cnode-start", 'delete'), ("cnode-delete", 'delete')]) ?*/
/*? include_task_type_replace([("cnode-start", 'invoke'), ("cnode-invoke", 'invoke')]) ?*/

    return 0;
}
/*-- endfilter -*/
```

```cmake
/*-- filter File("CMakeLists.txt") -*/
include(${SEL4_TUTORIALS_DIR}/settings.cmake)
sel4_tutorials_regenerate_tutorial(${CMAKE_CURRENT_SOURCE_DIR})

cmake_minimum_required(VERSION 3.7.2)
# declare the capabilities CMake project and the languages it is written in
project(capabilities C ASM)

sel4_tutorials_setup_roottask_tutorial_environment()

# Name the executable and list source files required to build it
add_executable(capabilities src/main.c)

# List of libraries to link with the application.
target_link_libraries(capabilities
    sel4
    muslc utils sel4tutorials
    sel4muslcsys sel4platsupport sel4utils sel4debug)

# Tell the build system that this application is the root task.
include(rootserver)
DeclareRootserver(capabilities)

# utility CMake functions for the tutorials (not required in normal, non-tutorial applications)
/*? macros.cmake_check_script(state) ?*/
/*-- endfilter -*/
```

/*-- endfilter -*/
