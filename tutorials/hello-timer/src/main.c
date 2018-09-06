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

#include <sel4platsupport/io.h>
#include <sel4platsupport/arch/io.h>
#include <sel4platsupport/timer.h>
#include <sel4platsupport/bootinfo.h>
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
seL4_timer_t timer;

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
    name_thread(seL4_CapInitThreadTCB, "hello-timer");

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

    /* copy the endpoint cap and add a badge to the new cap */
    new_ep_cap = sel4utils_mint_cap_to_process(&new_process, ep_cap_path,
                                               seL4_AllRights, EP_BADGE);
    assert(new_ep_cap != 0);

    /* spawn the process */
    error = sel4utils_spawn_process_v(&new_process, &vka, &vspace, 0, NULL, 1);
    assert(error == 0);

    /* TASK 1: create a notification endpoint for the timer interrupt */
    /* hint: vka_alloc_notification()
     * int vka_alloc_notification(vka_t *vka, vka_object_t *result)
     * @param vka Pointer to vka interface.
     * @param result Structure for the Endpoint object.  This gets initialised.
     * @return 0 on success
     * https://github.com/seL4/libsel4vka/blob/master/include/vka/object.h#L98
     */
/*- if solution -*/
    vka_object_t ntfn_object = {0};
    error = vka_alloc_notification(&vka, &ntfn_object);
    assert(error == 0);
/*- endif -*/

    /* TASK 2: call sel4platsupport library to get the default timer */
    /* hint: sel4platsupport_init_default_timer
     */
/*- if solution -*/
/*- raw -*/
    ps_io_ops_t ops = {{0}};
/*- endraw -*/
    error = sel4platsupport_new_io_ops(vspace, vka, &ops);
    assert(error == 0);

    error = sel4platsupport_new_arch_ops(&ops, &simple, &vka);
    assert(error == 0);

    error = sel4platsupport_init_default_timer_ops(&vka, &vspace, &simple, ops,
                                                   ntfn_object.cptr, &timer);
    assert(error == 0);
/*- endif -*/

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

    /*
     * TASK 3: Start and configure the timer
     * hint 1: ltimer_set_timeout
     * hint 2: set period to 1 millisecond
     */
/*- if solution -*/
    error = ltimer_set_timeout(&timer.ltimer, NS_IN_MS, TIMEOUT_PERIODIC);
    assert(error == 0);
/*- endif -*/

    int count = 0;
    while (1) {
        /*
         * TASK 4: Handle the timer interrupt
         * hint 1: wait for the incoming interrupt and handle it
         * The loop runs for (1000 * msg) times, which is basically 1 second * msg.
         *
         * hint2: seL4_Wait
         * hint3: sel4platsupport_handle_timer_irq
         *
         */
/*- if solution -*/
        seL4_Word badge;
        seL4_Wait(ntfn_object.cptr, &badge);
        sel4platsupport_handle_timer_irq(&timer, badge);
/*- endif -*/
        count++;
        if (count == 1000 * msg) {
            break;
        }
    }

    /* get the current time */
    uint64_t time;
    ltimer_get_time(&timer.ltimer, &time);

    /*
     * TASK 5: Stop the timer
     *
     * hint: sel4platsupport_destroy_timer
     *
     */
/*- if solution -*/
    sel4platsupport_destroy_timer(&timer, &vka);
/*- endif -*/

   /* modify the message */
    msg = (uint32_t) time;
    seL4_SetMR(0, msg);

    /* send the modified message back */
    seL4_ReplyRecv(ep_cap_path.capPtr, tag, &sender_badge);

    return 0;
}

