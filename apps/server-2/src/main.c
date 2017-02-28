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
 * seL4 RT tutorial part 1: Passive server thread
 */

/* Include Kconfig variables. */
#include <autoconf.h>

#include <stdio.h>
#include <assert.h>

#include <sel4/sel4.h>
#include <sel4/types_gen.h>

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

#include <utils/zf_log.h>
#include <sel4utils/sel4_zf_logif.h>

/* constants */
#define IPCBUF_FRAME_SIZE_BITS 12 // use a 4K frame for the IPC buffer
#define IPCBUF1_VADDR 0x7000000 // arbitrary (but free) address for IPC buffer
#define IPCBUF2_VADDR 0x7F00000

#define EP_BADGE 0x61 // arbitrary (but unique) number for a badge
#define MSG_DATA 0x6161 // arbitrary data to send

/* TODO_4: PLay around with these parameters and run the app. Try and see if you can explain what happens */
/* Some interesting examples are given below */

#define THREAD_1_BUDGET 1000
#define THREAD_1_PERIOD 1000
#define THREAD_1_PRIO   100

#define THREAD_2_BUDGET 1000
#define THREAD_2_PERIOD 1000
#define THREAD_2_PRIO   100

/* Example 1 */
/* Why does only thread 1 print? */
/*
#define THREAD_1_BUDGET 1000
#define THREAD_1_PERIOD 1000
#define THREAD_1_PRIO   101

#define THREAD_2_BUDGET 1000
#define THREAD_2_PERIOD 1000
#define THREAD_2_PRIO   100
*/

/* Example 2 */
/* Why does this behave differently to #1? */
/*
#define THREAD_1_BUDGET 500
#define THREAD_1_PERIOD 1000
#define THREAD_1_PRIO   101

#define THREAD_2_BUDGET 500
#define THREAD_2_PERIOD 1000
#define THREAD_2_PRIO   100
*/

/* Example 3 */
/* Why is this the same behaviour as #2? */
/*
#define THREAD_1_BUDGET 500
#define THREAD_1_PERIOD 1000
#define THREAD_1_PRIO   101

#define THREAD_2_BUDGET 1000
#define THREAD_2_PERIOD 1000
#define THREAD_2_PRIO   100
*/

/* Example 4 */
/* Why does thread 1 operate twice as fast as thread 2?*/
/*
#define THREAD_1_BUDGET 500
#define THREAD_1_PERIOD 1000
#define THREAD_1_PRIO   100

#define THREAD_2_BUDGET 250
#define THREAD_2_PERIOD 1000
#define THREAD_2_PRIO   100
*/


/* global environment variables */
seL4_BootInfo *info;
simple_t simple;
vka_t vka;
allocman_t *allocman;

/* variables shared with second thread */
vka_object_t ep_object;
vka_object_t sched_context1;
vka_object_t sched_context2;
cspacepath_t ep_cap_path;

/* static memory for the allocator to bootstrap with */
#define ALLOCATOR_STATIC_POOL_SIZE ((1 << seL4_PageBits) * 10)
UNUSED static char allocator_mem_pool[ALLOCATOR_STATIC_POOL_SIZE];

/* stack for the new thread */
#define THREAD_STACK_SIZE 512
static uint64_t thread_1_stack[THREAD_STACK_SIZE];
static uint64_t thread_2_stack[THREAD_STACK_SIZE];

/* convenience function */
extern void name_thread(seL4_CPtr tcb, char *name);

/* function to run in the new thread */
void thread_1(void) {
    int count = 0;
    while (1) {
        int a = 0;
        for (unsigned int i = 0; i < UINT_MAX / 20; i++) {
            a++;
        }
        printf("Thread 1: Tick %d\n", count);
        count++;
    }
}

void thread_2(void) {
    int count = 0;
    while (1) {
        int a = 0;
        for (unsigned int i = 0; i < UINT_MAX / 20; i++) {
            a++;
        }
        printf("Thread 2: Tick %d\n", count);
        count++;
    }
}

int main(void) {
    UNUSED int error;

    /* Set up logging and give us a name: useful for debugging if the thread faults */
    zf_log_set_tag_prefix("server-2:");
    name_thread(seL4_CapInitThreadTCB, "server-2");

    /* get boot info */
    info = seL4_GetBootInfo();

    /* init simple */
    simple_default_init_bootinfo(&simple, info);

    /* print out bootinfo and other info about simple */
    simple_print(&simple);

    /* create an allocator */
    allocman = bootstrap_use_current_simple(&simple, ALLOCATOR_STATIC_POOL_SIZE, allocator_mem_pool);
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

    /* create new TCBs */
    vka_object_t tcb_object1 = {0};
    vka_object_t tcb_object2 = {0};
    error = vka_alloc_tcb(&vka, &tcb_object1);
    error |= vka_alloc_tcb(&vka, &tcb_object2);
    ZF_LOGF_IFERR(error, "Failed to allocate new TCB.\n"
                  "\tVKA given sufficient bootstrap memory?");

    /* Create an endpoint */
    error = vka_alloc_endpoint(&vka, &ep_object);
    ZF_LOGF_IFERR(error, "Failed to allocate new endpoint object.\n");

    /*
     * create and map an ipc buffer:
     */

    /* Get a frame cap for the ipc buffer */
    vka_object_t ipc_frame_object1;
    vka_object_t ipc_frame_object2;
    error = vka_alloc_frame(&vka, IPCBUF_FRAME_SIZE_BITS, &ipc_frame_object1);
    error = vka_alloc_frame(&vka, IPCBUF_FRAME_SIZE_BITS, &ipc_frame_object2);
    ZF_LOGF_IFERR(error, "Failed to alloc a frame for the IPC buffer.\n"
                  "\tThe frame size is not the number of bytes, but an exponent.\n"
                  "\tNB: This frame is not an immediately usable, virtually mapped page.\n")
    /*
     * map the frame into the vspace at ipc_buffer_vaddr.
     * To do this we first try to map it in to the root page directory.
     * If there is already a page table mapped in the appropriate slot in the
     * page diretory where we can insert this frame, then this will succeed.
     * Otherwise we first need to create a page table, and map it in to
     * the page directory, before we can map the frame in. */

    seL4_Word ipc_buffer_vaddr1 = IPCBUF1_VADDR;
    seL4_Word ipc_buffer_vaddr2 = IPCBUF2_VADDR;
    /* Try to map the frame the first time  */
    error = seL4_ARCH_Page_Map(ipc_frame_object1.cptr, pd_cap, ipc_buffer_vaddr1,
                               seL4_AllRights, seL4_ARCH_Default_VMAttributes);
    error |= seL4_ARCH_Page_Map(ipc_frame_object2.cptr, pd_cap, ipc_buffer_vaddr2,
                                seL4_AllRights, seL4_ARCH_Default_VMAttributes);
    if (error != 0) {
        /* Create a page table */
        vka_object_t pt_object1;
        vka_object_t pt_object2;
        error = vka_alloc_page_table(&vka, &pt_object1);
        error |= vka_alloc_page_table(&vka, &pt_object2);
        ZF_LOGF_IFERR(error, "Failed to allocate new page table.\n");

        /* Map the page table */
        error = seL4_ARCH_PageTable_Map(pt_object1.cptr, pd_cap,
                                        ipc_buffer_vaddr1, seL4_ARCH_Default_VMAttributes);
        error |= seL4_ARCH_PageTable_Map(pt_object2.cptr, pd_cap,
                                         ipc_buffer_vaddr2, seL4_ARCH_Default_VMAttributes);
        ZF_LOGF_IFERR(error, "Failed to map page table into VSpace.\n"
                      "\tWe are inserting a new page table into the top-level table.\n"
                      "\tPass a capability to the new page table, and not for example, the IPC buffer frame vaddr.\n")

        /* then map the frame in */
        error = seL4_ARCH_Page_Map(ipc_frame_object1.cptr, pd_cap,
                                   ipc_buffer_vaddr1, seL4_AllRights, seL4_ARCH_Default_VMAttributes);
        error |= seL4_ARCH_Page_Map(ipc_frame_object2.cptr, pd_cap,
                                    ipc_buffer_vaddr2, seL4_AllRights, seL4_ARCH_Default_VMAttributes);
        ZF_LOGF_IFERR(error, "Failed again to map the IPC buffer frame into the VSpace.\n"
                      "\t(It's not supposed to fail.)\n"
                      "\tPass a capability to the IPC buffer's physical frame.\n"
                      "\tRevisit the first seL4_ARCH_Page_Map call above and double-check your arguments.\n");
    }

    /* set the IPC buffer's virtual address in a field of the IPC buffer */
    seL4_IPCBuffer *ipcbuf1 = (seL4_IPCBuffer*)ipc_buffer_vaddr1;
    seL4_IPCBuffer *ipcbuf2 = (seL4_IPCBuffer*)ipc_buffer_vaddr2;
    ipcbuf1->userData = ipc_buffer_vaddr1;
    ipcbuf2->userData = ipc_buffer_vaddr2;

    /* TODO 1: Create 2 caps to store 2 sched contexts */
    /* hint 1: vka_alloc_sched_context */

    /* TODO 2: initialise the scheduling contexts */
    /* hint 1: seL4_SchedControl_Configure */
    /* hint 2: simple_get_sched_ctrl */

    /* TODO 3: initialise the new TCBs */
    /* hint 1: seL4_TCB_Configure() */

    /* give the new threads names */
    name_thread(tcb_object1.cptr, "server-2: thread 1");
    name_thread(tcb_object2.cptr, "server-2: thread 2");

    /* set start up registers for the new thread */
    seL4_UserContext regs1 = {0};
    seL4_UserContext regs2 = {0};
    size_t regs_size = sizeof(seL4_UserContext) / sizeof(seL4_Word);

    /* set instruction pointer where the thread shoud start running */
    sel4utils_set_instruction_pointer(&regs1, (seL4_Word)thread_1);
    sel4utils_set_instruction_pointer(&regs2, (seL4_Word)thread_2);

    /* check that stack is aligned correctly */
    const int stack_alignment_requirement = sizeof(seL4_Word) * 2;
    uintptr_t thread_1_stack_top = (uintptr_t)thread_1_stack + sizeof(thread_1_stack);
    uintptr_t thread_2_stack_top = (uintptr_t)thread_2_stack + sizeof(thread_2_stack);

    ZF_LOGF_IF(thread_1_stack_top % (stack_alignment_requirement) != 0,
               "Stack top isn't aligned correctly to a %dB boundary.\n"
               "\tDouble check to ensure you're not trampling.",
               stack_alignment_requirement);
    ZF_LOGF_IF(thread_2_stack_top % (stack_alignment_requirement) != 0,
               "Stack top isn't aligned correctly to a %dB boundary.\n"
               "\tDouble check to ensure you're not trampling.",
               stack_alignment_requirement);

    /* set stack pointer for the new thread. remember the stack grows down */
    sel4utils_set_stack_pointer(&regs1, thread_1_stack_top);
    sel4utils_set_stack_pointer(&regs2, thread_2_stack_top);

    /* actually write the TCB registers. */
    error = seL4_TCB_WriteRegisters(tcb_object1.cptr, 0, 0, regs_size, &regs1);
    error |= seL4_TCB_WriteRegisters(tcb_object2.cptr, 0, 0, regs_size, &regs2);
    ZF_LOGF_IFERR(error, "Failed to write the new thread's register set.\n"
                  "\tDid you write the correct number of registers? See arg4.\n");

    /* start the new thread running */
    error = seL4_TCB_Resume(tcb_object1.cptr);
    error |= seL4_TCB_Resume(tcb_object2.cptr);
    ZF_LOGF_IFERR(error, "Failed to start new thread.\n");

    /* we are done, say hello */
    printf("main: hello world\n");
    seL4_Wait(ep_object.cptr, NULL);
    return 0;
}
