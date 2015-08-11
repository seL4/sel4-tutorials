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
 * Part 4: create a new process
 */


/* Include Kconfig variables. */
#include <autoconf.h>

#include <stdio.h>
#include <assert.h>

#include <sel4/sel4.h>

#include <simple/simple.h>
#include <simple-default/simple-default.h>

#include <vka/object.h>
//#include <vka/capops.h>
//#include <vka/vka.h>

#include <allocman/allocman.h>
#include <allocman/bootstrap.h>
#include <allocman/vka.h>

#include <vspace/vspace.h>

#include <sel4utils/vspace.h>
#include <sel4utils/mapping.h>
#include <sel4utils/process.h>

#define IPCBUF_FRAME_SIZE_BITS 12

#define EP_BADGE 0x61
#define MSG_DATA 0x6161

#define APP_PRIORITY 255
#define APP_IMAGE_NAME "hello-libs-4-apps"

seL4_BootInfo *info;
simple_t simple;
vka_t vka;
allocman_t *allocman;
vspace_t vspace;

/* dimensions of virtual memory for the allocator to use */
#define ALLOCATOR_VIRTUAL_POOL_SIZE ((1 << seL4_PageBits) * 100)

/* static memory for the allocator to bootstrap with */
#define ALLOCATOR_STATIC_POOL_SIZE ((1 << seL4_PageBits) * 10)
UNUSED static char allocator_mem_pool[ALLOCATOR_STATIC_POOL_SIZE];

/* static memory for virtual memory bootstrapping */
UNUSED static sel4utils_alloc_data_t data;

#define THREAD_2_STACK_SIZE 4096
UNUSED static int thread_2_stack[THREAD_2_STACK_SIZE];

void abort(void) {
    while (1);
}

void __arch_putchar(int c) {
#ifdef CONFIG_DEBUG_BUILD
    seL4_DebugPutChar(c);
#endif
}


int main(void)
{
    UNUSED int error;
    UNUSED void *ipcbuf_addr;
    UNUSED seL4_CPtr ipc_frame;

#ifdef SEL4_DEBUG_KERNEL
    seL4_DebugNameThread(seL4_CapInitThreadTCB, "hello-libs-4");
#endif

    /* get boot info */
    info = seL4_GetBootInfo();

    /* init simple */
    simple_default_init_bootinfo(&simple, info);

    /* Print bootinfo and other info about simple */
    simple_print(&simple);

    /* create an allocator */
    allocman = bootstrap_use_current_simple(&simple, ALLOCATOR_STATIC_POOL_SIZE, allocator_mem_pool);
    assert(allocman);

    /* create a vka (interface for interacting with the underlying allocator) */
    allocman_make_vka(&vka, allocman);

    /* create a vspace */
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


    /* find image code with cpio */
    // TODO

    /* use sel4utils to make a new process*/
    sel4utils_process_t new_process;
    error = sel4utils_configure_process(&new_process, &vka, &vspace,
            APP_PRIORITY, APP_IMAGE_NAME);
    assert(error == 0);

#ifdef SEL4_DEBUG_KERNEL
    seL4_DebugNameThread(new_process.thread.tcb.cptr, APP_IMAGE_NAME);
#endif

    /* create new EP */
    vka_object_t ep_object;
    ep_object.cptr = 0;

    error = vka_alloc_endpoint(&vka, &ep_object);
    assert(error == 0);

    cspacepath_t ep_cap_path;
    vka_cspace_make_path(&vka, ep_object.cptr, &ep_cap_path);
    
    seL4_CPtr new_ep_cap; 
    new_ep_cap = sel4utils_mint_cap_to_process(&new_process, ep_cap_path, seL4_AllRights, seL4_CapData_Badge_new(EP_BADGE));
    assert(new_ep_cap != 0);

    printf("main: new_ep_cap: %#x\n", new_ep_cap);

    /* spawn the process */
    error = sel4utils_spawn_process_v(&new_process, &vka, &vspace, 0, NULL, 1);
    assert(error == 0);

    /* send message on EP */
    seL4_Word msg;
    seL4_MessageInfo_t tag = seL4_MessageInfo_new(0, 0, 0, 1);
    seL4_SetMR(0, MSG_DATA);

    tag = seL4_Call(ep_cap_path.capPtr, tag);

    assert(seL4_MessageInfo_get_length(tag) == 1);
    msg = seL4_GetMR(0);
    assert(msg == ~MSG_DATA);

    printf("main: got a reply: %#x\n", msg);


    printf("main: hello world\n");

    return 0;
}

