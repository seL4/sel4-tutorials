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
 * seL4 tutorial part 3: IPC between 2 threads
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


/* constants */
#define IPCBUF_FRAME_SIZE_BITS 12 // use a 4K frame for the IPC buffer
#define IPCBUF_VADDR 0x7000000 // arbitrary (but free) address for IPC buffer

#define EP_BADGE 0x61 // arbitrary (but unique) number for a badge
#define MSG_DATA 0x6161 // arbitrary data to send

/* global environment variables */
seL4_BootInfo *info;
simple_t simple;
vka_t vka;
allocman_t *allocman;

/* static memory for the allocator to bootstrap with */
#define ALLOCATOR_STATIC_POOL_SIZE ((1 << seL4_PageBits) * 10)
UNUSED static char allocator_mem_pool[ALLOCATOR_STATIC_POOL_SIZE];

/* stack for the new thread */
#define THREAD_2_STACK_SIZE 512
static uint64_t thread_2_stack[THREAD_2_STACK_SIZE];

/* convenience function */
extern void name_thread(seL4_CPtr tcb, char *name);

/* function to run in the new thread */
void thread_2(void) {
    seL4_Word sender_badge;
    seL4_MessageInfo_t tag;
    seL4_Word msg;

    printf("thread_2: hallo wereld\n");

    /* TODO: wait for a message to come in over the endpoint
     * hint: seL4_Wait */

    /* TODO: get the message stored in the first message register
     * hint: seL$_GetMR() */
    printf("thread_2: got a message %#x from %#x\n", msg, sender_badge);

    /* modify the message */ 
    msg = ~msg;

    /* TODO: copy the modified message back into the message register
     * hint: seL4_SetMR() */

    /* TODO: send the message back: hint seL4_ReplyWait() */
}

int main(void)
{
    UNUSED int error;

    /* give us a name: useful for debugging if the thread faults */
    name_thread(seL4_CapInitThreadTCB, "hello-3");

    /* get boot info */
    info = seL4_GetBootInfo();

    /* init simple */
    simple_default_init_bootinfo(&simple, info);

    /* print out bootinfo and other info about simple */
    simple_print(&simple);

    /* create an allocator */
    allocman = bootstrap_use_current_simple(&simple, ALLOCATOR_STATIC_POOL_SIZE,        allocator_mem_pool);
    assert(allocman);

    /* create a vka (interface for interacting with the underlying allocator) */
    allocman_make_vka(&vka, allocman);

    /* get our cspace root cnode */
    seL4_CPtr cspace_cap;
    cspace_cap = simple_get_cnode(&simple);

    /* get our vspace root page directory */
    seL4_CPtr pd_cap;
    pd_cap = simple_get_pd(&simple);

    /* create a new TCB */
    vka_object_t tcb_object = {0};
    error = vka_alloc_tcb(&vka, &tcb_object);
    assert(error == 0);

    /*
     * create and map an ipc buffer:
     */

    /* TODO: get a frame cap for the ipc buffer: hint vka_alloc_frame() */

    /*
     * TODO: map the frame into the vspace at ipc_buffer_vaddr.
     * To do this we first try to map it in to the root page directory.
     * If there is already a page table mapped in the appropriate slot in the
     * page diretory where we can insert this frame, then this will succeed.
     * Otherwise we first need to create a page table, and map it in to
     * the page directory, before we can map the frame in.
     * hint 1: seL4_ARCH_Page_Map() 
     * hint 1b: seL4_AllRights seL4_ARCH_Default_VMAttributes
     * hint 2: vka_alloc_page_table()
     * hint 3: seL4_ARCH_PageTable_Map()
     */

    /* set the IPC buffer's virtual address in a field of the IPC buffer */
    seL4_IPCBuffer *ipcbuf = (seL4_IPCBuffer*)ipc_buffer_vaddr;
    ipcbuf->userData = ipc_buffer_vaddr;

    /* TODO: create an endpoint: hint vka_alloc_endpoint() */

    /* TODO: make a badged copy of it in our cspace.
     * This copy will be used to send an IPC message to the original cap
     * hint 1: vka_mint_object()
     * hint 2: seL4_AllRights()
     * hint 3: seL4_CapData_Badge_new()
     */

    /* initialise the new TCB */
    error = seL4_TCB_Configure(tcb_object.cptr, seL4_CapNull, seL4_MaxPrio,
        cspace_cap, seL4_NilData, pd_cap, seL4_NilData,
        ipc_buffer_vaddr, ipc_frame_object.cptr);
    assert(error == 0);

    /* give the new thread a name */
    name_thread(tcb_object.cptr, "hello-3: thread_2");

    /* set start up registers for the new thread */
    seL4_UserContext regs = {0};
    size_t regs_size = sizeof(seL4_UserContext) / sizeof(seL4_Word);

    /* set instruction pointer where the thread shoud start running */
    sel4utils_set_instruction_pointer(&regs, (seL4_Word)thread_2);

    /* check that stack is aligned correctly */
    uintptr_t thread_2_stack_top = (uintptr_t)thread_2_stack + sizeof(thread_2_stack);
    assert(thread_2_stack_top % (sizeof(seL4_Word) * 2) == 0);

    /* set stack pointer for the new thread. remember the stack grows down */
    sel4utils_set_stack_pointer(&regs, thread_2_stack_top);

    /* set the gs register for thread local storage */
    regs.gs = IPCBUF_GDT_SELECTOR;

    /* actually write the TCB registers. */
    error = seL4_TCB_WriteRegisters(tcb_object.cptr, 0, 0, regs_size, &regs);
    assert(error == 0);

    /* start the new thread running */
    error = seL4_TCB_Resume(tcb_object.cptr);
    assert(error == 0);

    /* we are done, say hello */
    printf("main: hello world\n");

    /*
     * now send a message to the new thread, and wait for a reply
     */

    /* TODO: set the data to send. We send it in the first message register
     * hint 1: seL4_MessageInfo_new()
     * hint 2: seL4_SetMR() */

    /* TODO: send and wait for a reply: hint seL4_Call() */

    /* TODO: get the reply message: hint seL4_GetMR() */

    printf("main: got a reply: %#x\n", msg);

    return 0;
}

