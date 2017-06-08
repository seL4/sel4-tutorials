/*
 * Copyright 2017, Data61
 * Commonwealth Scientific and Industrial Research Organisation (CSIRO)
 * ABN 41 687 119 230.
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(DATA61_BSD)
 */

/*
 * seL4 tutorial part 5: using the real time features of the MCS kernel
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

#include <utils/arith.h>
#include <utils/time.h>
#include <utils/zf_log.h>
#include <sel4utils/sel4_zf_logif.h>

#include <sel4platsupport/bootinfo.h>
#include <sel4platsupport/timer.h>

/* constants */
#define APP_PRIORITY seL4_MaxPrio
#define NUM_YIELDS 5

/* global environment variables */
seL4_BootInfo *info;
simple_t simple;
vka_t vka;
allocman_t *allocman;
vspace_t vspace;
seL4_timer_t *timer;

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

/* a thread that just prints and yields forever */
void yielding_thread(void)
{
    int i = 0;
    while (true) {
        printf("Pong %d\n", i);
        /* Yield for a round robin thread (where budget == period) will apply round-robin scheduling.
         * However for a periodic thread, it will charge the current thread's all of remaining budget
         * and the thread will block until it has more budget available */
        seL4_Yield();
        i++;
        ZF_LOGF_IF(i > NUM_YIELDS * 3, "Too many yields!");
    }
}

void sender(seL4_CPtr ep)
{
    int i = 0;
    while (true) {
        seL4_MessageInfo_t info = seL4_MessageInfo_new(0, 0, 0, 1);
        seL4_SetMR(0, i);
        seL4_Send(ep, info);
        printf("%d\n", i);
        i++;
    }
}

/* a very basic and inefficient IPC echo server */
void echo_server(seL4_CPtr ep, seL4_CPtr reply)
{
    char message[seL4_MsgMaxLength + 1];
    /* We do an NBSendRecv first, in order to tell our initialiser
     * that we are ready to be converted to passive, and block on an
     * endpoint in the one go */
    seL4_MessageInfo_t info = seL4_MessageInfo_new(0, 0, 0, 0);

    /* TASK 9: signal to the main thread that the echo server is initialised
     * and ready to be converted to passive. Block in the same system call. */

    /* hint: seL4_NBSendRecv can send a non-blocking message and block in one system call
     * hint: you can use the same ep for both the NBSend and Recv phase */


    /* At this point we have got our first message -> process it and wait for more */
    while (true) {
        int i;
        for (i = 0; i < seL4_MessageInfo_get_length(info); i++) {
            /* we buffer the message as calls to printf can overwrite the
             * ipc buffer - in this system as it goes by the kernel (using seL4_DebugPutChar) to
             * eventually print - but in other systems that use serial servers too */
            message[i] = (char) seL4_GetMR(i);
        }
        message[i] = '\0';
        printf("echo: %s\n", message);
        info = seL4_ReplyRecv(ep, info, NULL, reply);
    }
}

int main(void) {
    UNUSED int error;

    /* get boot info */
    info = platsupport_get_bootinfo();
    ZF_LOGF_IF(info == NULL, "Failed to get bootinfo.");

    /* Set up logging and give us a name: useful for debugging if the thread faults */
    zf_log_set_tag_prefix("mcs");
    name_thread(seL4_CapInitThreadTCB, "mcs");

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

    /* create a vspace object to manage our vspace */
    error = sel4utils_bootstrap_vspace_with_bootinfo_leaky(&vspace,
                                                           &data, simple_get_pd(&simple), &vka, info);
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


    /* create a timer for use in the tutorials */
    vka_object_t ntfn_object = {0};
    error = vka_alloc_notification(&vka, &ntfn_object);
    ZF_LOGF_IF(error != 0, "Failed to allocate notification");

    timer = sel4platsupport_get_default_timer(&vka, &vspace, &simple, ntfn_object.cptr);
    ZF_LOGF_IF(timer == NULL, "Failed to get default timer");

    seL4_CPtr sched_control = seL4_CapNull;
   /* TASK 1: obtain the scheduling control capability for the current node, which allows a
    * scheduling context to be configured for that node. */

    /* hint: simple_get_sched_ctrl
     * hint: seL4_BootInfo has id of the current core */


    ZF_LOGF_IF(sched_control == seL4_CapNull, "Failed to find sched_control.");

    /* TASK 2: create a scheduling context object */

    /* Scheduling contexts represent time on a specific CPU node. On creation, their parameters
     * are set to 0 and they do not represent any time. */
    vka_object_t sc_object = {0};
    /* hint: vka_alloc_sched_context_size */

    ZF_LOGF_IFERR(error, "Failed to allocate new SC");


    /* TASK 3: Use the sched control capability to configure the scheduling context for
     * a round robin thread with a 10ms timeslice */

    /* hint: seL4_SchedControl_Configure
     * hint: time.h in libutils has constants for time.
     * hint: For round-robin threads, the budget and period should be thread same
     */

    ZF_LOGF_IFERR(error, "Failed to configure new sc");


    /* create a round-robin thread at our own priority */
    sel4utils_thread_t thread;
    error = sel4utils_configure_thread(&vka, &vspace, &vspace, seL4_CapNull, APP_PRIORITY,
                                       sc_object.cptr, simple_get_cnode(&simple),
                                       seL4_NilData, &thread);
    ZF_LOGF_IF(error != seL4_NoError, "Failed to configure thread");
    name_thread(thread.tcb.cptr, "helper_thread");

    /* start the thread */
    error = sel4utils_start_thread(&thread, (sel4utils_thread_entry_fn) yielding_thread, NULL, NULL, true);
    ZF_LOGF_IFERR(error, "Failed to start thread");

    /* Yield to the round robin thread a few times, to show it is running */
    printf("=== Round robin ===\n");
    for (int i = 0; i < NUM_YIELDS; i++) {
        printf("Ping %d\n", i);
        seL4_Yield();
    }

    /* TASK 4: Stop the thread by unbinding its scheduling context */

    /* hint: seL4_SchedContext_Unbind or seL4_SchedContext_UnbindObject will do the job */

    ZF_LOGF_IFERR(error, "Failed to unbind scheduling context");

    /* Yield to the round robin thread a few times, to show it isn't running */
    printf("=== Just Ping ===\n");
    for (int i = 0; i < NUM_YIELDS; i++) {
        printf("Ping\n");
        seL4_Yield();
    }

    /* TASK 5: reconfigure the scheduling context to be periodic with period 2s and budget 1s  */

    ZF_LOGF_IFERR(error, "Failed to configure new sc");

    /* set up the timer */
    timer_start(timer->timer);
    /* the PIT used on qemu cannot handle large timeouts, so set
     * a small timeout (1ms) and count the number of ms until a second goes by */
    timer_periodic(timer->timer, NS_IN_MS);

    printf("=== Periodic ===\n");
    /* Set our priority down so we don't interrupt the high priority thread */
    error = seL4_TCB_SetPriority(simple_get_tcb(&simple), APP_PRIORITY - 1);
    ZF_LOGF_IFERR(error, "Failed to set our own priority");

   /* TASK 6: rebind the scheduling context to the thread we created */
    /* hint: seL4_SchedContext_Bind
     * hint: thread.tcb.cptr can be used to get a capability to the TCB of the thread
     * we created with sel4utils_configure_thread. */

    ZF_LOGF_IFERR(error, "Failed to bind sc to thread");

    int ticks_per_tick = NS_IN_S / NS_IN_MS;
    for (int i = 0; i < 10 * ticks_per_tick; i++) {
        seL4_Wait(ntfn_object.cptr, NULL);
        sel4_timer_handle_single_irq(timer);
        if (i % ticks_per_tick == 0) {
            printf("Tick\n");
        }
    }
    timer_stop(timer->timer);

    /* First set the other thread to a lower priority, we need to be higher priority for the next task */
    error = seL4_TCB_SetPriority(thread.tcb.cptr, APP_PRIORITY - 2);
    ZF_LOGF_IFERR(error, "Failed to set thread's priority");

    /* TASK 7 - reconfigure the thread and set the extra_refills parameter to 2 */

    ZF_LOGF_IFERR(error, "Failed to reconfigure scheduling context");

    /* create an ep */
    vka_object_t ep = {0};
    error = vka_alloc_endpoint(&vka, &ep);
    ZF_LOGF_IFERR(error, "Failed to allocate ep");

    /* restart the thread as an echo server */
    sel4utils_start_thread(&thread, (sel4utils_thread_entry_fn) sender, (void *) ep.cptr, NULL, true);

    printf("== Sporadic ==\n");
    for (int i = 0; i < NUM_YIELDS * 2; i++) {
        seL4_Wait(ep.cptr, NULL);
        printf("%u\n", (unsigned int) (rdtsc_pure() / NS_IN_S));
    }

    /* now stop the thread */
    error = seL4_TCB_Suspend(thread.tcb.cptr);
    ZF_LOGF_IFERR(error, "Failed to stop thread");


    /* TASK 8 - create a reply object - reply objects are new in the MCS kernel and allow scheduling context
     * donation to occur over IPC - they are required to use Call/ReplyRecv pairs */
    /* hint: vka_alloc_reply */
    vka_object_t reply = {0};

    ZF_LOGF_IFERR(error, "Failed to allocate reply");

    /* Now we set up thread as a passive echo server */
    sel4utils_start_thread(&thread, (sel4utils_thread_entry_fn) echo_server, (void *) ep.cptr, (void *) reply.cptr, true);

    /* wait for a message from the server to say it is initialised */
    printf("Waiting for server\n");
    seL4_Wait(ep.cptr, NULL);

    /* delete the scheduling context so the server can't use it -
     * this is one way to convert the server to passive */
    vka_free_object(&vka, &sc_object);

    /* Now the server is passive - because we converted it to passive once it was waiting on
     * an endpoint it can now receive scheduling contexts over IPC */

    /* set our priority down - we are going to be the client
     * of a server using IPCP (ICPP on wikipedia: https://en.wikipedia.org/wiki/Priority_ceiling_protocol)
     * which requires the server to be the
     * highest priority of all its clients */
    error = seL4_TCB_SetPriority(thread.tcb.cptr, APP_PRIORITY - 3);
    ZF_LOGF_IFERR(error, "Failed to set TCB priority");

    /* Send the server a message to make sure its working */
    const char *messages[] = {
        "running passive echo server",
        "2nd message processed",
        "mcs tutorial finished!",
    };

    for (int m = 0; m < ARRAY_SIZE(messages); m++) {
        int i;
        for (i = 0; i < strlen(messages[m]) && i < seL4_MsgMaxLength; i++) {
            seL4_SetMR(i, messages[m][i]);
        }
        seL4_MessageInfo_t info = seL4_MessageInfo_new(0, 0, 0, i);
        seL4_Call(ep.cptr, info);
    }

    return 0;
}