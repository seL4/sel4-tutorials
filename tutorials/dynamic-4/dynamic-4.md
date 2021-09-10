<!--
  Copyright 2017, Data61, CSIRO (ABN 41 687 119 230)

  SPDX-License-Identifier: BSD-2-Clause
-->

/*? declare_task_ordering(
['task-1',
'task-2',
'task-3',
'task-4',
'task-5'
])?*/
# seL4 Dynamic Libraries: Timer tutorial

This tutorial demonstrates how to set up and use a basic timer driver using the
`seL4_libs` dynamic libraries.

You'll observe that the things you've already covered in the other
tutorials are already filled out and you don't have to repeat them: in
much the same way, we won't be repeating conceptual explanations on this
page, if they were covered by a previous tutorial in the series.

## Learning outcomes

- Allocate a notification object.
- Set up a timer provided by `util_libs`.
- Use `seL4_libs` and `util_libs` functions to manipulate timer and
      handle interrupts.

## Initialising

/*? macros.tutorial_init("dynamic-4") ?*/

## Prerequisites

1. [Set up your machine](https://docs.sel4.systems/HostDependencies).
1. [dynamic-3](https://docs.sel4.systems/Tutorials/dynamic-3)

## Exercises

Once you initialise and run the tutorials, you will see the following output:

```
Booting all finished, dropped to user space
Node 0 of 1
IOPT levels:     4294967295
IPC buffer:      0x5a1000
Empty slots:     [523 --> 4096)
sharedFrames:    [0 --> 0)
userImageFrames: [16 --> 433)
userImagePaging: [12 --> 15)
untypeds:        [433 --> 523)
Initial thread domain: 0
Initial thread cnode size: 12
timer client: hey hey hey
main: hello world
main: got a message from 0x61 to sleep 2 seconds
ltimer_get_time@ltimer.h:267 get_time not implemented
/*-- filter TaskCompletion("task-1", TaskContentType.ALL) -*/
timer client wakes up:
 got the current timer tick:
 0
/*-- endfilter -*/
```
### Allocate a notification object

The first task is to allocate a notification object to receive
interrupts on.
```c
/*-- set task_1_desc -*/
    /* TASK 1: create a notification object for the timer interrupt */
    /* hint: vka_alloc_notification()
     * int vka_alloc_notification(vka_t *vka, vka_object_t *result)
     * @param vka Pointer to vka interface.
     * @param result Structure for the notification object. This gets initialised.
     * @return 0 on success
     * https://github.com/seL4/libsel4vka/blob/master/include/vka/object.h#L98
     */
    vka_object_t ntfn_object = {0};
/*-- endset -*/
/*? task_1_desc ?*/
/*-- filter TaskContent("task-1", TaskContentType.BEFORE) -*/
/*-- endfilter -*/
/*-- filter ExcludeDocs() -*/
/*-- filter TaskContent("task-1", TaskContentType.COMPLETED) -*/
    error = vka_alloc_notification(&vka, &ntfn_object);
    assert(error == 0);
/*-- endfilter -*/
/*-- endfilter -*/
```
The output will not change as a result of completing this task.

### Initialise the timer

Use our library function `ltimer_default_init` to
initialise a timer driver. Assign it to the `timer` global variable.
```c
/*-- set task_2_desc -*/
    /* TASK 2: call ltimer library to get the default timer */
    /* hint: ltimer_default_init, you can set NULL for the callback and token
     */
    ps_io_ops_t ops = {{0}};
    error = sel4platsupport_new_malloc_ops(&ops.malloc_ops);
    assert(error == 0);
    error = sel4platsupport_new_io_mapper(&vspace, &vka, &ops.io_mapper);
    assert(error == 0);
    error = sel4platsupport_new_fdt_ops(&ops.io_fdt, &simple, &ops.malloc_ops);
    assert(error == 0);
    if (ntfn_object.cptr != seL4_CapNull) {
        error = sel4platsupport_new_mini_irq_ops(&ops.irq_ops, &vka, &simple, &ops.malloc_ops,
                                                 ntfn_object.cptr, MASK(seL4_BadgeBits));
        assert(error == 0);
    }
    error = sel4platsupport_new_arch_ops(&ops, &simple, &vka);
    assert(error == 0);
/*-- endset -*/
/*? task_2_desc ?*/
/*-- filter ExcludeDocs() -*/
/*-- filter TaskContent("task-2", TaskContentType.COMPLETED) -*/
    error = ltimer_default_init(&timer, ops, NULL, NULL); 
    assert(error == 0);
/*-- endfilter -*/
/*-- endfilter -*/
```
After this change, the server will output non-zero for the tick value at the end.
```
/*-- filter TaskCompletion("task-2", TaskContentType.COMPLETED) -*/
 got the current timer tick:
/*-- endfilter -*/
 1409040
```

### Use the timer

While at the end of the previous task the tutorial appears to be
working, the main thread replies immediately to the client and doesn't
wait at all.

Consequently, the final task is to interact with the timer: set a
timeout, wait for an interrupt on the created notification, and handle
it.

```c
/*-- set task_3_desc -*/
    /*
     * TASK 3: Start and configure the timer
     * hint 1: ltimer_set_timeout
     * hint 2: set period to 1 millisecond
     */
/*-- endset -*/
/*? task_3_desc ?*/
/*-- filter ExcludeDocs() -*/
/*-- filter TaskContent("task-3", TaskContentType.COMPLETED) -*/
    error = ltimer_set_timeout(&timer, NS_IN_MS, TIMEOUT_PERIODIC);
    assert(error == 0);
/*-- endfilter -*/
/*-- endfilter -*/
```
The output will cease after the following line as a result of completing this task.
```
/*-- filter TaskCompletion("task-3", TaskContentType.COMPLETED) -*/
main: got a message from 0x61 to sleep 2 seconds
/*-- endfilter -*/
```

### Handle the interrupt

In order to receive more interrupts, you need to handle the interrupt in the driver
and acknowledge the irq. 

```c
/*-- set task_4_desc -*/
        /*
         * TASK 4: Handle the timer interrupt
         * hint 1: wait for the incoming interrupt and handle it
         * The loop runs for (1000 * msg) times, which is basically 1 second * msg.
         *
         * hint2: seL4_Wait
         * hint3: sel4platsupport_irq_handle
         * hint4: 'ntfn_id' should be MINI_IRQ_INTERFACE_NTFN_ID and handle_mask' should be the badge
         *
         */
/*-- endset -*/
/*? task_4_desc ?*/
/*-- filter ExcludeDocs() -*/
/*-- filter TaskCompletion("task-4", TaskContentType.COMPLETED) -*/
main: got a message from 0x61 to sleep 2 seconds
/*-- endfilter -*/
/*-- filter TaskContent("task-4", TaskContentType.COMPLETED) -*/
        seL4_Word badge;
        seL4_Wait(ntfn_object.cptr, &badge);
        sel4platsupport_irq_handle(&ops.irq_ops, MINI_IRQ_INTERFACE_NTFN_ID, badge);
/*-- endfilter -*/
/*-- endfilter -*/
```
The timer interrupts are bound to the IRQ interface initialised in Task 2,
hence when we receive an interrupt, we forward it to the interface and let it notify the timer driver.

After this task is completed you should see a 2 second wait, then output from the
 client as follows:
```
/*-- filter TaskCompletion("task-4", TaskContentType.COMPLETED) -*/
timer client wakes up:
 got the current timer tick:
/*-- endfilter -*/
 2365866120
```

### Destroy the timer

```c
/*-- set task_5_desc -*/
    /*
     * TASK 5: Stop the timer
     * hint: ltimer_destroy 
     */
/*-- endset -*/
/*? task_5_desc ?*/
/*-- filter ExcludeDocs() -*/
/*-- filter TaskCompletion("task-5", TaskContentType.COMPLETED) -*/
timer client wakes up:
 got the current timer tick:
/*-- endfilter -*/
/*-- filter TaskContent("task-5", TaskContentType.COMPLETED) -*/
    ltimer_destroy(&timer);
/*-- endfilter -*/
/*-- endfilter -*/
```
The output should not change on successful completion of completing this task.

That's it for this tutorial.

/*? macros.help_block() ?*/
/*-- filter ExcludeDocs() -*/
/*? ExternalFile("CMakeLists.txt") ?*/
```
/*-- filter File("main.c") -*/
/*
 * Copyright 2018, Data61, CSIRO (ABN 41 687 119 230).
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */
/* Include Kconfig variables. */
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

#include <sel4platsupport/io.h>
#include <sel4platsupport/irq.h>
#include <sel4platsupport/arch/io.h>
#include <sel4platsupport/bootinfo.h>
#include <platsupport/plat/timer.h>
#include <platsupport/ltimer.h>

/* constants */
#define EP_BADGE 0x61 // arbitrary (but unique) number for a badge
#define MSG_DATA 0x6161 // arbitrary data to send

#define APP_PRIORITY seL4_MaxPrio
#define APP_IMAGE_NAME "client"

/* global environment variables */
seL4_BootInfo *info;
simple_t simple;
vka_t vka;
allocman_t *allocman;
vspace_t vspace;
ltimer_t timer;

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

/* convenience function */
extern void name_thread(seL4_CPtr tcb, char *name);

int main(void) {
    UNUSED int error;

    /* get boot info */
    info = platsupport_get_bootinfo();
    ZF_LOGF_IF(info == NULL, "Failed to get bootinfo.");

    /* give us a name: useful for debugging if the thread faults */
    name_thread(seL4_CapInitThreadTCB, "dynamic-4");

    /* init simple */
    simple_default_init_bootinfo(&simple, info);

    /* print out bootinfo and other info about simple */
    simple_print(&simple);

    /* create an allocator */
    allocman = bootstrap_use_current_simple(&simple, ALLOCATOR_STATIC_POOL_SIZE,
                                            allocator_mem_pool);
    assert(allocman);

    /* create a vka (interface for interacting with the underlying allocator) */
    allocman_make_vka(&vka, allocman);

    /* create a vspace object to manage our vspace */
    error = sel4utils_bootstrap_vspace_with_bootinfo_leaky(&vspace,
                                                           &data, simple_get_pd(&simple), &vka, info);

    /* fill the allocator with virtual memory */
    void *vaddr;
    UNUSED reservation_t virtual_reservation;
    virtual_reservation = vspace_reserve_range(&vspace,
                                               ALLOCATOR_VIRTUAL_POOL_SIZE, seL4_AllRights, 1, &vaddr);
    assert(virtual_reservation.res);
    bootstrap_configure_virtual_pool(allocman, vaddr,
                                     ALLOCATOR_VIRTUAL_POOL_SIZE, simple_get_pd(&simple));

    /* use sel4utils to make a new process */
    sel4utils_process_t new_process;
    sel4utils_process_config_t config = process_config_default_simple(&simple, APP_IMAGE_NAME, APP_PRIORITY);
    config = process_config_auth(config, simple_get_tcb(&simple));
    config = process_config_priority(config, seL4_MaxPrio);
    error = sel4utils_configure_process_custom(&new_process, &vka, &vspace, config);
    assert(error == 0);

    /* give the new process's thread a name */
    name_thread(new_process.thread.tcb.cptr, "dynamic-4: timer_client");

    /* create an endpoint */
    vka_object_t ep_object = {0};
    error = vka_alloc_endpoint(&vka, &ep_object);
    assert(error == 0);

    /*
     * make a badged endpoint in the new process's cspace.  This copy
     * will be used to send an IPC to the original cap
     */

    /* make a cspacepath for the new endpoint cap */
    cspacepath_t ep_cap_path;
    seL4_CPtr new_ep_cap;
    vka_cspace_make_path(&vka, ep_object.cptr, &ep_cap_path);

    /* copy the endpoint cap and add a badge to the new cap */
    new_ep_cap = sel4utils_mint_cap_to_process(&new_process, ep_cap_path,
                                               seL4_AllRights, EP_BADGE);
    assert(new_ep_cap != 0);

    /* spawn the process */
    error = sel4utils_spawn_process_v(&new_process, &vka, &vspace, 0, NULL, 1);
    assert(error == 0);

    /*? task_1_desc ?*/
    /*? include_task_type_append([("task-1")]) ?*/
    /*? task_2_desc ?*/
    /*? include_task_type_append([("task-2")]) ?*/

    /* we are done, say hello */
    printf("main: hello world\n");

    /*
     * now wait for a message from the new process, then send a reply back
     */

    seL4_Word sender_badge;
    seL4_MessageInfo_t tag;
    seL4_Word msg;

    /* wait for a message */
    tag = seL4_Recv(ep_cap_path.capPtr, &sender_badge);

    /* make sure it is what we expected */
    assert(sender_badge == EP_BADGE);
    assert(seL4_MessageInfo_get_length(tag) == 1);

    /* get the message stored in the first message register */
    msg = seL4_GetMR(0);
    printf("main: got a message from %#" PRIxPTR " to sleep %zu seconds\n", sender_badge, msg);

    /*? task_3_desc ?*/
    /*? include_task_type_append([("task-3")]) ?*/

    int count = 0;
    while (1) {

        /*? task_4_desc ?*/
        /*? include_task_type_append([("task-4")]) ?*/
        count++;
        if (count == 1000 * msg) {
            break;
        }
    }

    /* get the current time */
    uint64_t time = 0;
    ltimer_get_time(&timer, &time);

    /*? task_5_desc ?*/
    /*? include_task_type_append([("task-5")]) ?*/

    /* modify the message */
    msg = (uint32_t) time;
    seL4_SetMR(0, msg);

    /* send the modified message back */
    seL4_ReplyRecv(ep_cap_path.capPtr, tag, &sender_badge);

    return 0;
}
/*-- endfilter -*/
/*-- filter File("client.c") -*/
/*
 * Copyright 2017, Data61, CSIRO (ABN 41 687 119 230).
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */
#include <stdio.h>
#include <assert.h>

#include <sel4/sel4.h>
#include <sel4utils/process.h>

/* constants */
#define EP_CPTR SEL4UTILS_FIRST_FREE // where the cap for the endpoint was placed.
#define MSG_DATA 0x2 //  arbitrary data to send

int main(int argc, char **argv) {
    seL4_MessageInfo_t tag;
    seL4_Word msg;

    printf("timer client: hey hey hey\n");

    /* set the data to send. We send it in the first message register */
    tag = seL4_MessageInfo_new(0, 0, 0, 1);
    seL4_SetMR(0, MSG_DATA);

    /* send a message to our parent, and wait for a reply */
    tag = seL4_Call(EP_CPTR, tag);

    /* check that we got the expected repy */
    assert(seL4_MessageInfo_get_length(tag) == 1);
    msg = seL4_GetMR(0);

    printf("timer client wakes up:\n got the current timer tick:\n %zu\n", msg);

    return 0;
}
/*- endfilter -*/
```
/*- endfilter -*/
