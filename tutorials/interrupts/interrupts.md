<!--
  Copyright 2017, Data61, CSIRO (ABN 41 687 119 230)

  SPDX-License-Identifier: BSD-2-Clause
-->

# Interrupts
/*? declare_task_ordering(['timer-start', 'timer-get', 'timer-set', 'timer-ack']) ?*/

## Prerequisites

1. [Set up your machine](https://docs.sel4.systems/HostDependencies).
2. [Notification tutorial](https://docs.sel4.systems/Tutorials/notifications)

# Initialising

/*? macros.tutorial_init("interrupts") ?*/

## Outcomes

* Understand the purpose of the IRQControl capability.
* Be able to obtain capabilities for specific interrupts.
* Learn how to handle interrupts and their relation with notification objects.

## Background

### IRQControl

The root task is given a single capability from which capabilities to all irq numbers
in the system can be derived, `seL4_CapIRQControl`. This capability can be moved between CSpaces
and CSlots but cannot be duplicated. Revoking this capability results in all access to all
irq capabilities being removed.

### IRQHandlers

IRQHandler capabilities give access to a single irq and are standard seL4 capabilities: they
*can* be moved and duplicated according to system policy. IRQHandlers are obtained by
invoking the IRQControl capability, with architecture specific parameters. Below is an
example of obtaining an IRQHandler.

```bash
// Get a capability for irq number 7 and place it in cslot 10 in a single-level cspace.
error = seL4_IRQControl_Get(seL4_IRQControl, 7, cspace_root, 10, seL4_WordBits);
```

There are a variety of different invocations to obtain irq capabilities which are hardware
dependent, including:

* [`seL4_IRQControl_GetIOAPIC`](https://docs.sel4.systems/ApiDoc.html#get-io-apic) (x86)
* [`seL4_IRQControl_GetMSI`](https://docs.sel4.systems/ApiDoc.html#get-msi) (x86)
* [`seL4_IRQControl_GetTrigger`](https://docs.sel4.systems/ApiDoc.html#gettrigger) (ARM)

### Receiving interrupts

Interrupts are received by registering a capability to a notification object
with the IRQHandler capability for that irq, as follows:
```bash
seL4_IRQHandler_SetNotification(irq_handler, notification);
```
On success, this call will result in signals being delivered to the notification object when
an interrupt occurs. To handle multiple interrupts on the same notification object, you
can set different badges on the notification capabilities bound to each IRQHandler.
 When an interrupt arrives,
the badge of the notification object bound to that IRQHandler is bitwise orred with the data
word in the notification object.
Recall the badging technique for differentiating signals from the
 [notification tutorial](https://docs.sel4.systems/Tutorials/notifications).

Interrupts can be polled for using `seL4_Poll` or waited for using `seL4_Wait`. Either system
call results in the data word of the notification object being delivered as the badge of the
message, and the data word cleared.

[`seL4_IRQHandler_Clear`](https://docs.sel4.systems/ApiDoc.html#clear) can be used to unbind
the notification from an IRQHandler.

### Handling interrupts

Once an interrupt is received and processed by the software, you can unmask the interrupt
using [`seL4_IRQHandler_Ack`](https://docs.sel4.systems/ApiDoc.html#ack) on the IRQHandler.
seL4 will not deliver any further interrupts after an IRQ is raised until that IRQHandler
has been acked.

## Exercises

In this tutorial you will set up interrupt handling for a provided timer driver
on the zynq7000 ARM platform. This timer driver can be located inside the
`projects/sel4-tutorials/zynq_timer_driver` folder from the root of the
projects directory, i.e. where the `.repo` folder can be found and where the
initial `repo init` command was executed. The tutorial has been set up with two
processes: `timer.c`, the timer driver and RPC server, and `client.c`, which
makes a single request.

On successful initialisation of the tutorial, you will see the following:

```
timer client: hey hey hey
timer: got a message from 61 to sleep 2 seconds
<<seL4(CPU 0) [decodeInvocation/530 T0xe8265600 "tcb_timer" @84e4]: Attempted to invoke a null cap #9.>>
main@timer.c:78 [Cond failed: error]
/*-- filter TaskCompletion("timer-start", TaskContentType.ALL) -*/
	Failed to ack irq
/*-- endfilter -*/
```

The timer driver we are using emits an interrupt in the `TTC0_TIMER1_IRQ` number.

**Exercise** Invoke `irq_control`, which contains the `seL4_IRQControl` capability,
the place the `IRQHandler` capability for `TTC0_TIMER1_IRQ` into the `irq_handler` CSlot.

```
/*-- filter TaskContent("timer-start", TaskContentType.ALL, subtask='get') -*/
    /* TODO invoke irq_control to put the interrupt for TTC0_TIMER1_IRQ in
       cslot irq_handler (depth is seL4_WordBits) */
/*-- endfilter -*/
/*-- filter ExcludeDocs() -*/
/*-- filter TaskContent("timer-get", TaskContentType.COMPLETED, subtask='get') -*/
    /* put the interrupt handle for TTC0_TIMER1_IRQ in the irq_handler cslot */
    error = seL4_IRQControl_Get(irq_control, TTC0_TIMER1_IRQ, cnode, irq_handler, seL4_WordBits);
    ZF_LOGF_IF(error, "Failed to get irq capability");
/*-- endfilter -*/
/*-- endfilter -*/
```

On success, you should see the following output, without the error message that occurred earlier,
as the irq_handle capability is now valid:

```
/*-- filter TaskCompletion("timer-get", TaskContentType.COMPLETED) -*/
Undelivered IRQ: 42
/*-- endfilter -*/
```

This is a warning message from the kernel that an IRQ was recieved for irq number 42, but no
notification capability is set to sent a signal to.

**Exercise** Now set the notification capability (`ntfn`) by invoking the irq handler.

```
/*-- filter TaskContent("timer-start", TaskContentType.ALL, subtask='set') -*/
     /* TODO set ntfn as the notification for irq_handler */
/*-- endfilter -*/
/*-- filter ExcludeDocs() -*/
/*-- filter TaskContent("timer-set", TaskContentType.COMPLETED, subtask='set') -*/
    /* set ntfn as the notification for irq_handler */
    error =  seL4_IRQHandler_SetNotification(irq_handler, ntfn);
    ZF_LOGF_IF(error, "Failed to set notification");
/*-- endfilter -*/
/*-- endfilter -*/
```

Now the output will be:

```
/*-- filter TaskCompletion("timer-set", TaskContentType.COMPLETED) -*/
Tick
/*-- endfilter -*/
```

Only one interrupt is delivered, as the interrupt has not been acknowledged. The timer driver is
programmed to emit an interrupt every millisecond, so we need to count 2000 interrupts
before replying to the client.

**Exercise** Acknowledge the interrupt after handling it in the timer driver.

```
/*-- filter TaskContent("timer-start", TaskContentType.ALL, subtask='ack') -*/
        /* TODO ack the interrupt */
/*-- endfilter -*/
/*-- filter ExcludeDocs() -*/
/*-- filter TaskContent("timer-ack", TaskContentType.COMPLETED, subtask='ack') -*/
        /* ack the interrupt */
        error = seL4_IRQHandler_Ack(irq_handler);
        ZF_LOGF_IF(error, "Failed to ack irq");
/*-- endfilter -*/
/*-- endfilter -*/
```

Now the timer interrupts continue to come in, and the reply is delivered to the client.

```
/*-- filter TaskCompletion("timer-ack", TaskContentType.COMPLETED) -*/
timer client wakes up
/*-- endfilter -*/
```

That's it for this tutorial.

/*? macros.help_block() ?*/

/*-- filter ExcludeDocs() -*/
```c
/*-- filter ELF("client") -*/
/*- set _ = state.stash.start_elf("client") -*/
#include <stdio.h>
#include <assert.h>

#include <sel4/sel4.h>
#include <sel4utils/process.h>

/* constants */
// cslot containing the endpoint for the server
/*? capdl_alloc_cap(seL4_EndpointObject, "endpoint", "endpoint", write=True, read=True, grant=True, badge=61) ?*/
#define MSG_DATA 0x2 //  arbitrary data to send

int main(int argc, char **argv) {
    seL4_MessageInfo_t tag;
    seL4_Word msg;

    printf("timer client: hey hey hey\n");

    /*
     * send a message to our parent, and wait for a reply
     */

    /* set the data to send. We send it in the first message register */
    tag = seL4_MessageInfo_new(0, 0, 0, 1);
    seL4_SetMR(0, MSG_DATA);

    /* send and wait for a reply */
    tag = seL4_Call(endpoint, tag);

    /* check that we got the expected repy */
    assert(seL4_MessageInfo_get_length(tag) == 1);
    msg = seL4_GetMR(0);
    assert(msg == 0);

    printf("timer client wakes up\n");

    return 0;
}
/*-- endfilter -*/
/*-- filter ELF("timer") -*/
/*- set _ = state.stash.start_elf("timer") -*/
#include <stdio.h>
#include <assert.h>
#include <sel4/sel4.h>
#include <timer_driver/driver.h>

// CSlots pre-initialised in this CSpace
/*? capdl_alloc_cap(seL4_EndpointObject, "endpoint", "endpoint", write=True, read=True, grant=True) ?*/
// capability to a reply object
/*? capdl_alloc_cap(seL4_NotificationObject, "ntfn", "ntfn", write=True, read=True, grant=True) ?*/
// capability to the device untyped for the timer
/*- set _ = capdl_alloc_obj(seL4_UntypedObject, "device_untyped", paddr=4160753664) -*/
/*? capdl_alloc_cap(seL4_UntypedObject, "device_untyped", "device_untyped", write=True, read=True) ?*/
// empty cslot for the frame
/*? capdl_empty_slot("timer_frame") ?*/
// cnode of this process
/*? capdl_elf_cspace("timer", "cnode") ?*/
// vspace of this process
/*? capdl_elf_vspace("timer", "vspace") ?*/
// frame to map the timer to
/*? capdl_declare_frame("frame", "timer_vaddr") ?*/
// irq control capability
/*? capdl_irq_control("irq_control") ?*/
// empty slot for the irq
/*? capdl_empty_slot("irq_handler") ?*/

/* constants */
#define EP_BADGE 61 // arbitrary (but unique) number for a badge
#define MSG_DATA 0x6161 // arbitrary data to send

#define DEFAULT_TIMER_ID 0
#define TTC0_TIMER1_IRQ 42

int main(void) {
    /* wait for a message */
    seL4_Word sender_badge;
    seL4_MessageInfo_t tag = seL4_Recv(endpoint, &sender_badge);

    /* make sure the message is what we expected */
    assert(sender_badge == EP_BADGE);
    assert(seL4_MessageInfo_get_length(tag) == 1);

    /* get the message stored in the first message register */
    seL4_Word msg = seL4_GetMR(0);
    printf("timer: got a message from %u to sleep %zu seconds\n", sender_badge, msg);

    /* retype the device untyped into a frame */
    seL4_Error error = seL4_Untyped_Retype(device_untyped, seL4_ARM_SmallPageObject, 0,
                                          cnode, 0, 0, timer_frame, 1);
    ZF_LOGF_IF(error, "Failed to retype device untyped");

    /* unmap the existing frame mapped at vaddr so we can map the timer here */
    error = seL4_ARM_Page_Unmap(frame);
    ZF_LOGF_IF(error, "Failed to unmap frame");

    /* map the device frame into the address space */
    error = seL4_ARM_Page_Map(timer_frame, vspace, (seL4_Word) timer_vaddr, seL4_AllRights, 0);
    ZF_LOGF_IF(error, "Failed to map device frame");

    timer_drv_t timer_drv = {0};

    /*? include_task_type_replace([("timer-start", 'get'), ("timer-get", 'get')]) ?*/
    /*? include_task_type_replace([("timer-start", 'set'), ("timer-set", 'set')]) ?*/

    /* set up the timer driver */
    int timer_err = timer_init(&timer_drv, DEFAULT_TIMER_ID, (void *) timer_vaddr);
    ZF_LOGF_IF(timer_err, "Failed to init timer");

    timer_err = timer_start(&timer_drv);
    ZF_LOGF_IF(timer_err, "Failed to start timer");

    /* ack the irq in case of any pending interrupts int the driver */
    error = seL4_IRQHandler_Ack(irq_handler);
    ZF_LOGF_IF(error, "Failed to ack irq");

    timer_err = timer_set_timeout(&timer_drv, NS_IN_MS, true);
    ZF_LOGF_IF(timer_err, "Failed to set timeout");

    int count = 0;
    while (1) {
        /* Handle the timer interrupt */
        seL4_Word badge;
        seL4_Wait(ntfn, &badge);
        timer_handle_irq(&timer_drv);
        if (count == 0) {
            printf("Tick\n");
        }
        /*? include_task_type_replace([("timer-start", 'ack'), ("timer-ack", 'ack')]) ?*/

        count++;
        if (count == 1000 * msg) {
            break;
        }
    }

    // stop the timer
    timer_stop(&timer_drv);

   /* modify the message */
    seL4_SetMR(0, 0);

    /* send the modified message back */
    seL4_ReplyRecv(endpoint, tag, &sender_badge);

    return 0;
}


/*-- endfilter -*/
```
/*? ExternalFile("CMakeLists.txt") ?*/
/*- endfilter -*/
