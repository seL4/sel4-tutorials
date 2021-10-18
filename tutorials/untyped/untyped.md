<!--
  Copyright 2017, Data61, CSIRO (ABN 41 687 119 230)

  SPDX-License-Identifier: BSD-2-Clause
-->

/*? declare_task_ordering(['untyped-start','untyped-next','untyped-tcb','untyped-ep','untyped-ntfn','untyped-revoke']) ?*/

# Untyped

This tutorial provides an introduction to physical memory management on seL4.

## Prerequisites

1. [Set up your machine](https://docs.sel4.systems/HostDependencies).
1. [Capabilities tutorial](https://docs.sel4.systems/Tutorials/capabilities)

## Outcomes

By the end of this tutorial, you should be familiar with:

1. The jargon *untyped*, *device untyped*, and *bit size*.
2. Know how to create objects from untyped memory in seL4.
3. Know how to reclaim objects.

## Background

### Physical memory

Apart from a small, static amount of kernel memory, all physical memory is managed by user level in an seL4
system. Capabilities to objects created by seL4 at boot, as well as the rest of the physical resources managed by seL4,
are passed to the root task on start up.

### Untyped memory and capabilities

Excluding the objects used to create the root task, capabilities to all
available physical memory are passed to the root task as capabilities to
*untyped* memory. Untyped memory is a block of contiguous physical memory with a
specific size. Untyped capabilities are capabilities to untyped memory. Untyped
capabilities can be *retyped* into kernel objects together with capabilities to
them, or into further, usually smaller, untyped capabilities.

Untyped capabilities have a boolean property *device* which indicates whether
the memory is writable by the kernel or not: it may not be backed by RAM but
some other device, or it may be in an area of RAM not addressable by the kernel.
Device untyped capabilities can only be retyped intro frame objects (physical
memory frames, which can be mapped into virtual memory), and cannot be written
to by the kernel.



### Initial state

The `seL4_BootInfo` structure provided to the root task describes all of the untyped capabilities, including their size,
if they are a device untyped, and the physical address of the untyped. The code example below
shows how to print out the initial untyped capabilities provided from `seL4_BootInfo`.

```c
/*-- filter TaskContent("untyped-start", TaskContentType.ALL, subtask='init') -*/
    printf("    CSlot   \tPaddr           \tSize\tType\n");
    for (seL4_CPtr slot = info->untyped.start; slot != info->untyped.end; slot++) {
        seL4_UntypedDesc *desc = &info->untypedList[slot - info->untyped.start];
        printf("%8p\t%16p\t2^%d\t%s\n", (void *) slot, (void *) desc->paddr, desc->sizeBits, desc->isDevice ? "device untyped" : "untyped");
    }
/*-- endfilter -*/
```

### Retyping

Untyped capabilities have a single invocation:
[seL4_Untyped_Retype](https://docs.sel4.systems/ApiDoc.html#retype) which is
used to create a new capability (and potentially object) from an untyped
capability. Specifically, the new capability created by a retype invocation
provides access to a subset of the memory range of the original capability,
either as a smaller untyped capability, or pointing to a new object with a
specific type. New capabilities created by retyping an untyped capability are
referred to as *children* of that untyped capability.

Untyped capabilities are retyped incrementally in a greedy fashion from the
invoked untyped, which is important to understand in order to gain efficient
memory utilisation in seL4 systems. Each untyped capability maintains a single
watermark, with addresses before the watermark being unavailable (already
retyped) and after the watermark being free (not retyped yet). On a retype
operation, the watermark is moved first to the alignment of the object being
created, and then to the end of the size of the object. For example, if we
create first a 4KiB object and then a 16KiB object, the 12KiB between the end of
the 4KiB object and the start of the 16KiB object are wasted due to alignment.
The memory cannot be reclaimed until both children are revoked.

TL;DR: objects should be allocated in order of size, largest first, to avoid wasting memory.

```c
/*-- filter TaskContent("untyped-start", TaskContentType.ALL, subtask='retype') -*/
    error = seL4_Untyped_Retype(parent_untyped, // the untyped capability to retype
                                seL4_UntypedObject, // type
                                untyped_size_bits,  //size
                                seL4_CapInitThreadCNode, // root
                                0, // node_index
                                0, // node_depth
                                child_untyped, // node_offset
                                1 // num_caps
                                );
/*-- endfilter -*/
```

The above code snippet shows an example of retyping an untyped into a smaller,
4KiB untyped capability (despite the name `seL4_UntypedObject` no object is
created, the new untyped capability simply points to a subset of the untyped
memory of `parent_untyped`. However, it is convenient to treat this memory like
an object for the purposes of this operation). We will refer to this snippet as
we discuss each parameter in the retype invocation. Recall that the first
parameter is the untyped capability being invoked to do the retype.

#### Types

Each object in seL4 has a specific type, and all of the type constants can be found in `libsel4`. Some types
are architecture specific, while others are general across architectures. In the example above, we create a
new untyped capability, which can be further retyped. All other object type parameters create kernel objects *and*
capabilities to them, and the type of the created object (and capability) determines the invocations
that can be made on that capability. For example, were we to retype into a thread control block
(TCB -- `seL4_TCBObject`), we could then perform TCB invocations on the capability to the new TCB object, including `seL4_TCB_SetPriority`.

#### Size

The size argument determines the size of the new object. The meaning of the argument
depends on the object type that is being requested:

- most objects in seL4 are fixed size, and for these, the kernel will *ignore*
  the size argument;
- the types `seL4_UntypedObject` and `seL4_SchedContextObject` allow a
  variable size, which is specified in `size_bits` (more below);
- the type `seL4_CapTableObject` is also of variable size, and the argument specifies
  the *number of capability slots*. The same mechanism as for `size_bits` is used,
  but for the number of slots, not the size in bytes.

In general in seL4, if sizes are measured in bits, they are powers
of two. "Bits"  does not refer to the number of bits the object takes up, but
the bit-width that is needed to describe its contents. An object of bit size `n`
measures 2<sup>`n`</sup> bytes. Also, generally in seL4, objects and memory
regions are aligned to their size, i.e. an `n`-bit object is aligned to
2<sup>`n`</sup> bytes, or, equivalently, the address of the object has 0 as its
`n` bottom bits.

For `retype`, it is enough to remember that the parameter `size_bits` means the
object will measure 2<sup>`size_bits`</sup> bytes, and for `seL4_CapTableObject`
that you are requesting 2<sup>`size_bits`</sup> slots (you can compute the size
in bytes by taking 2<sup>`size_bits + seL4_SlotBits`</sup>).

#### Root, node_index & node_depth

The `root`, `node_index` and `node_depth` parameters are used to specify the CNode in which to place the new capabilities.
Depending on the `depth` parameter, the CSlot to use is addressed by invocation or by direct addressing (see the [capabilities tutorial](https://docs.sel4.systems/Tutorials/capabilities.html) for an explanation of those terms).

In the example above, `node_depth` is set to 0, which means invocation addressing is used: the `root` parameter is
looked up implicitly using the CSpace root of the current thread at a depth of `seL4_WordBits`. So the example code
specifies the root task's CNode (`seL4_CapInitThreadCNode`). The `node_index` parameter in this case is ignored.

If the `node_depth` value is not set to 0, then direct addressing is used with the current thread's CSpace root
as the root. Then the `node_index` parameter is used to locate the CNode capability to place new capabilities in,
at the specified `node_depth`. This is designed for managing multi-level CSpaces, and is not covered in this tutorial.

#### Node_offset

The node_offset is the CSlot to start creating new capabilities at, in the CNode selected by the previous parameters.
 In this case, the first empty CSlot in the initial CNode is selected.

#### Num_caps

The retype invocation can be used to create more than 1 capability and object at a time -- the number of capabilities is
specified using this argument. Note that there are two constraints on this value:

1. The untyped must be big enough to fit all of the memory being retyped (`num_caps * (1u << size_bits)`).
2. The CNode must have enough consecutive free CSlots to fit all of the new capabilities.

## Exercises

### Create an untyped capability

When you first run this tutorial, you will see something like the following output, which lists all of the
untyped capabilities provided to the root task on boot:

```
Booting all finished, dropped to user space
    CSlot   Paddr       Size    Type
   0x12d    0x100000    2^20    device untyped
   0x12e    0x200000    2^21    device untyped
   0x12f    0x400000    2^22    device untyped
   0x130    0xb2e000    2^13    device untyped
```

At the end of the output, there is an error:

```
<<seL4(CPU 0) [decodeUntypedInvocation/105 T0xffffff801ffb5400 "rootserver" @4012e1]: Untyped Retype: Requested UntypedItem size too small.>>
main@main.c:49 [Cond failed: error != seL4_NoError]
    Failed to retype
```

This error happens because we are trying to create an untyped of size 0.

**Exercise** Calculate the size of the child untyped required, such that the child untyped can be used
 to create all of the objects listed in the `objects` array. Remember that object sizes are measured in
 bits, that is, in powers of two.

```c
/*-- filter TaskContent("untyped-start", TaskContentType.ALL, subtask='setup', completion='Failed to retype') -*/
    // list of general seL4 objects
    seL4_Word objects[] = {seL4_TCBObject, seL4_EndpointObject, seL4_NotificationObject};
    // list of general seL4 object size_bits
    seL4_Word sizes[] = {seL4_TCBBits, seL4_EndpointBits, seL4_NotificationBits};

    // TODO work out what size object we need to create to be able to create all of the objects
    // listed above. Remember that sizes are in bits, that is, the exponents of powers of two.
    seL4_Word untyped_size_bits = 0;
    seL4_CPtr parent_untyped = 0;
    seL4_CPtr child_untyped = info->empty.start;

    // First, find an untyped big enough to fit all of our objects
    for (int i = 0; i < (info->untyped.end - info->untyped.start); i++) {
        if (info->untypedList[i].sizeBits >= untyped_size_bits && !info->untypedList[i].isDevice) {
            parent_untyped = info->untyped.start + i;
            break;
        }
    }
/*-- endfilter -*/
/*-- filter ExcludeDocs() -*/
/*-- filter TaskContent("untyped-next", TaskContentType.COMPLETED, subtask='setup', completion='Failed to set priority') -*/
    // list of general seL4 objects
    seL4_Word objects[] = {seL4_TCBObject, seL4_EndpointObject, seL4_NotificationObject};
    // list of general seL4 object size_bits
    seL4_Word sizes[] = {seL4_TCBBits, seL4_EndpointBits, seL4_NotificationBits};

    // seL4_EndpointBits and seL4_NotificationBits are both less than seL4_TCBBits, which
    // means that all objects together fit into the size of two TCBs, or 2^(seL4_TCBBits + 1):
    seL4_Word untyped_size_bits = seL4_TCBBits + 1;
    seL4_CPtr parent_untyped = 0;
    seL4_CPtr child_untyped = info->empty.start;

    // First, find an untyped big enough to fit all of our objects
    for (int i = 0; i < (info->untyped.end - info->untyped.start); i++) {
        if (info->untypedList[i].sizeBits >= untyped_size_bits && !info->untypedList[i].isDevice) {
            parent_untyped = info->untyped.start + i;
            break;
        }
    }
/*-- endfilter -*/
/*-- endfilter -*/
```

On success, the tutorial will progress further, printing "Failed to set priority"

### Create a TCB Object

The priority check is failing as `child_tcb` is an empty CSlot.

**Exercise** fix this by creating a TCB object from `child_untyped` and place its cap into the `child_tcb` CSlot.

```c
/*-- filter TaskContent("untyped-start", TaskContentType.ALL, subtask='tcb') -*/
    // use the slot after child_untyped for the new TCB cap:
    seL4_CPtr child_tcb = child_untyped + 1;
    /* TODO create a TCB in CSlot child_tcb */

    // try to set the TCB priority
    error = seL4_TCB_SetPriority(child_tcb, seL4_CapInitThreadTCB, 10);
    ZF_LOGF_IF(error != seL4_NoError, "Failed to set priority");
/*-- endfilter -*/
```

/*-- filter ExcludeDocs() -*/

```c
/*-- filter TaskContent("untyped-tcb", TaskContentType.COMPLETED, subtask='tcb', completion='Endpoint cap is null cap') -*/
    // use the slot after child_untyped for the new TCB cap:
    seL4_CPtr child_tcb = child_untyped + 1;
    seL4_Untyped_Retype(child_untyped, seL4_TCBObject, 0, seL4_CapInitThreadCNode, 0, 0, child_tcb, 1);

    // try to set the TCB priority
    error = seL4_TCB_SetPriority(child_tcb, seL4_CapInitThreadTCB, 10);
    ZF_LOGF_IF(error != seL4_NoError, "Failed to set priority");
/*-- endfilter -*/
```

/*-- endfilter -*/

On success, the tutorial will progress further, printing "Endpoint cap is null cap".

### Create an endpoint object

The error you see now is caused be an invalid endpoint capability.

**Exercise** Create an endpoint object from `child_untyped` and place its cap into the `child_ep` CSlot.

```c
/*-- filter TaskContent("untyped-start", TaskContentType.ALL, subtask='ep') -*/
    // use the slot after child_tcb for the new endpoint cap:
    seL4_CPtr child_ep = child_tcb + 1;
    /* TODO create an endpoint in CSlot child_ep */

    // identify the type of child_ep
    uint32_t cap_id = seL4_DebugCapIdentify(child_ep);
    ZF_LOGF_IF(cap_id == 0, "Endpoint cap is null cap");
/*-- endfilter -*/
/*-- filter ExcludeDocs() -*/
/*-- filter TaskContent("untyped-ep", TaskContentType.COMPLETED, subtask='ep', completion='Failed to bind notification.') -*/
    // use the slot after child_tcb for the new endpoint cap:
    seL4_CPtr child_ep = child_tcb + 1;
    seL4_Untyped_Retype(child_untyped, seL4_EndpointObject, 0, seL4_CapInitThreadCNode, 0, 0, child_ep, 1);

    // identify the type of child_ep
    uint32_t cap_id = seL4_DebugCapIdentify(child_ep);
    ZF_LOGF_IF(cap_id == 0, "Endpoint cap is null cap");
/*-- endfilter -*/
/*-- endfilter -*/
```

On success, 'Failed to bind notification' should be output.

### Create a notification object

The next part of the tutorial attempts to use a notification object that does not yet exist.

**Exercise** create a notification object from `child_untyped` and place its cap into the `child_ntfn` CSlot.

```c
/*-- filter TaskContent("untyped-start", TaskContentType.ALL, subtask='ntfn') -*/
    // use the slot after child_ep for the new notification cap:
    seL4_CPtr child_ntfn = child_ep + 1;
    // TODO create a notification object in CSlot child_ntfn

    // try to use child_ntfn
    error = seL4_TCB_BindNotification(child_tcb, child_ntfn);
    ZF_LOGF_IF(error != seL4_NoError, "Failed to bind notification.");
/*-- endfilter -*/
```

/*-- filter ExcludeDocs() -*/

```c
/*-- filter TaskContent("untyped-ntfn", TaskContentType.COMPLETED, subtask='ntfn', completion='Failed to create endpoints..') -*/
    // use the slot after child_ep for the new notification cap:
    seL4_CPtr child_ntfn = child_ep + 1;
    seL4_Untyped_Retype(child_untyped, seL4_NotificationObject, 0, seL4_CapInitThreadCNode, 0, 0, child_ntfn, 1);

    // try to use child_ntfn
    error = seL4_TCB_BindNotification(child_tcb, child_ntfn);
    ZF_LOGF_IF(error != seL4_NoError, "Failed to bind notification.");
/*-- endfilter -*/
```

/*-- endfilter -*/

### Delete the objects

The final part of the tutorial attempts to create enough endpoint objects from `child_untyped` to consume the
entire untyped object. However, this fails, because the untyped is already completely consumed by the previous allocations.

**Exercise** revoke the child untyped, so we can create new objects from it that use up the whole thing.

```c
/*-- filter TaskContent("untyped-start", TaskContentType.ALL, subtask='revoke') -*/
    // TODO revoke the child untyped

    // allocate the whole child_untyped as endpoints
    // Remember the sizes are exponents, so this computes 2^untyped_size_bits / 2^seL4_EndpointBits:
    seL4_Word num_eps = BIT(untyped_size_bits - seL4_EndpointBits);
    error = seL4_Untyped_Retype(child_untyped, seL4_EndpointObject, 0, seL4_CapInitThreadCNode, 0, 0, child_tcb, num_eps);
    ZF_LOGF_IF(error != seL4_NoError, "Failed to create endpoints.");

    printf("Success\n");
/*-- endfilter -*/
/*-- filter ExcludeDocs() -*/
/*-- filter TaskContent("untyped-revoke", TaskContentType.COMPLETED, subtask='revoke', completion='Success') -*/
    error = seL4_CNode_Revoke(seL4_CapInitThreadCNode, child_untyped, seL4_WordBits);
    assert(error == seL4_NoError);

    // allocate the whole child_untyped as endpoints
    // Remember the sizes are exponents, so this computes 2^untyped_size_bits / 2^seL4_EndpointBits:
    seL4_Word num_eps = BIT(untyped_size_bits - seL4_EndpointBits);
    error = seL4_Untyped_Retype(child_untyped, seL4_EndpointObject, 0, seL4_CapInitThreadCNode, 0, 0, child_tcb, num_eps);
    ZF_LOGF_IF(error != seL4_NoError, "Failed to create endpoints.");

    printf("Success\n");
/*-- endfilter -*/
/*-- endfilter -*/
```

Once the tutorial is completed successfully, you should see the message "Success".

### Further exercises

That's all for the detailed content of this tutorial. Below we list other ideas for exercises you can try
to become more familiar with untyped objects and memory allocation in seL4.

* Allocate objects at specific physical addresses.
* Create a simple object allocator for allocating seL4 objects.

/*? macros.help_block() ?*/

/*-- filter ExcludeDocs() -*/

```c
/*- filter File("src/main.c") -*/
#include <stdio.h>
#include <sel4/sel4.h>
#include <sel4platsupport/bootinfo.h>
#include <utils/util.h>

int main(int argc, char *argv[]) {
    /* parse the location of the seL4_BootInfo data structure from
    the environment variables set up by the default crt0.S */
    seL4_BootInfo *info = platsupport_get_bootinfo();

/*? include_task_type_append([("untyped-start", 'init')]) ?*/
    seL4_Error error;
/*? include_task_type_replace([("untyped-start", 'setup'), ("untyped-next", 'setup')]) ?*/
    // create an untyped big enough to retype all of the above objects from
/*? include_task_type_append([("untyped-start", 'retype')]) ?*/
    ZF_LOGF_IF(error != seL4_NoError, "Failed to retype");
/*? include_task_type_replace([("untyped-start", 'tcb'), ("untyped-tcb", 'tcb')]) ?*/
/*? include_task_type_replace([("untyped-start", 'ep'), ("untyped-ep", 'ep')]) ?*/
/*? include_task_type_replace([("untyped-start", 'ntfn'), ("untyped-ntfn", 'ntfn')]) ?*/
/*? include_task_type_replace([("untyped-start", 'revoke'), ("untyped-revoke", 'revoke')]) ?*/

    return 0;
}
/*- endfilter -*/
```

```cmake
/*- filter File("CMakeLists.txt") -*/
include(${SEL4_TUTORIALS_DIR}/settings.cmake)
sel4_tutorials_regenerate_tutorial(${CMAKE_CURRENT_SOURCE_DIR})

cmake_minimum_required(VERSION 3.7.2)
project(untyped C ASM)

sel4_tutorials_setup_roottask_tutorial_environment()

# Name the executable and list source files required to build it
add_executable(untyped src/main.c)

# List of libraries to link with the application.
target_link_libraries(untyped
    sel4
    muslc utils sel4tutorials
    sel4muslcsys sel4platsupport sel4utils sel4debug)

# Tell the build system that this application is the root task.
include(rootserver)
DeclareRootserver(untyped)

# utility CMake functions for the tutorials (not required in normal, non-tutorial applications)
/*? macros.cmake_check_script(state) ?*/
/*- endfilter -*/
```

/*-- endfilter -*/
