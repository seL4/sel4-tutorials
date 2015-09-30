/*
 * Copyright 2015, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

/*
 * seL4 tutorial part 4: create a new process and IPC with it
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

#include <sel4platsupport/timer.h>
#include <platsupport/plat/timer.h>

/* constants */
#define EP_BADGE 0x61 // arbitrary (but unique) number for a badge
#define MSG_DATA 0x6161 // arbitrary data to send

#define APP_PRIORITY seL4_MaxPrio
#define APP_IMAGE_NAME "hello-timer-client"

/* global environment variables */
seL4_BootInfo *info;
simple_t simple;
vka_t vka;
allocman_t *allocman;
vspace_t vspace;
seL4_timer_t *timer;

/* static memory for the allocator to bootstrap with */
#define ALLOCATOR_STATIC_POOL_SIZE ((1 << seL4_PageBits) * 10)
UNUSED static char allocator_mem_pool[ALLOCATOR_STATIC_POOL_SIZE];

/* dimensions of virtual memory for the allocator to use */
#define ALLOCATOR_VIRTUAL_POOL_SIZE ((1 << seL4_PageBits) * 100)

/* static memory for virtual memory bootstrapping */
UNUSED static sel4utils_alloc_data_t data;

/* stack for the new thread */
#define THREAD_2_STACK_SIZE 4096
UNUSED static int thread_2_stack[THREAD_2_STACK_SIZE];

/* convenience function */
extern void name_thread(seL4_CPtr tcb, char *name);

int main(void)
{
    UNUSED int error;

    /* give us a name: useful for debugging if the thread faults */
    name_thread(seL4_CapInitThreadTCB, "hello-timer");

    /* get boot info */
    info = seL4_GetBootInfo();

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
    error = sel4utils_configure_process(&new_process, &vka, &vspace,
        APP_PRIORITY, APP_IMAGE_NAME);
    assert(error == 0);

    /* give the new process's thread a name */
    name_thread(new_process.thread.tcb.cptr, "hello-timer: timer_client");

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

    /* copy the endpont cap and add a badge to the new cap */
    new_ep_cap = sel4utils_mint_cap_to_process(&new_process, ep_cap_path,
        seL4_AllRights, seL4_CapData_Badge_new(EP_BADGE));
    assert(new_ep_cap != 0);

    /* spawn the process */
    error = sel4utils_spawn_process_v(&new_process, &vka, &vspace, 0, NULL, 1);
    assert(error == 0);

    /* TODO 1: create an endpoint for the timer interrupt */
    vka_object_t aep_object = {0};
    error = vka_alloc_async_endpoint(&vka, &aep_object);
    assert(error == 0);

    /* TODO 2: get the default timer */
    timer = sel4platsupport_get_default_timer(&vka, &vspace, &simple, aep_object.cptr);
    assert(timer != NULL);

    /* we are done, say hello */
    printf("main: hello world\n");

    /*
     * now wait for a message from the new process, then send a reply back
     */

    seL4_Word sender_badge;
    seL4_MessageInfo_t tag;
    seL4_Word msg;

    /* wait for a message */
    tag = seL4_Wait(ep_cap_path.capPtr, &sender_badge);

    /* make sure it is what we expected */
    assert(sender_badge == EP_BADGE);
    assert(seL4_MessageInfo_get_length(tag) == 1);

    /* get the message stored in the first message register */
    msg = seL4_GetMR(0);
    printf("main: got a message from %#x to sleep %u seconds\n", sender_badge, msg);

    /* TODO 3: wait for the timeout */
    seL4_Word sender;
    int count = 0;
    while(1) {
        timer_oneshot_relative(timer->timer, 1000 * 1000);
        seL4_Wait(aep_object.cptr, &sender);
        sel4_timer_handle_single_irq(timer);
        count++;
        if (count == 1000 * msg) break;
    }

    /* get the current time */
    msg = timer_get_time(timer->timer);

    /* modify the message */
    seL4_SetMR(0, msg);

    /* send the modified message back */
    seL4_ReplyWait(ep_cap_path.capPtr, tag, &sender_badge);

    return 0;
}

