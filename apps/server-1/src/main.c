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
#define IPCBUF_VADDR 0x7000000 // arbitrary (but free) address for IPC buffer

#define EP_BADGE 0x61 // arbitrary (but unique) number for a badge
#define MSG_DATA 0x6161 // arbitrary data to send

/* global environment variables */
seL4_BootInfo *info;
simple_t simple;
vka_t vka;
allocman_t *allocman;

/* variables shared with second thread */
vka_object_t ep_object;
vka_object_t sched_context;
cspacepath_t ep_cap_path;

/* static memory for the allocator to bootstrap with */
#define ALLOCATOR_STATIC_POOL_SIZE ((1 << seL4_PageBits) * 10)
UNUSED static char allocator_mem_pool[ALLOCATOR_STATIC_POOL_SIZE];

/* stack for the new thread */
#define SERVER_STACK_SIZE 512
static uint64_t server_stack[SERVER_STACK_SIZE];

/* convenience function */
extern void name_thread(seL4_CPtr tcb, char *name);

/* Do some intialisation of variables in our server */
void server_init(int *a, int *b, int *c) {
    *a = 1;
    *b = 2;
    *c = 3;
}

/* function to run in the new thread */
void server(void) {
    int a, b, c;
    /* do some initialisation before we start accepting requests*/
    server_init(&a, &b, &c);
    seL4_MessageInfo_t msg = seL4_MessageInfo_new(0, 0, 0, 0);
    printf("server: finished init\n");
    /* TODO 5: Make this server passive */
    /* This will be done by unbinding the scheduling context once we are done
       with initialisation. For the initialiser to do this, we must tell it
       the server is finished with init */
    /* hint 1: seL4_SignalRecv */

    /*  We are now ready */
    printf("server: hello world\n");
    while (1) {
        seL4_ReplyRecv(ep_object.cptr, msg, NULL);
        printf("server: Received IPC, data %08X\n", seL4_GetMR(0));
    }

}



int main(void) {
    UNUSED int error;

    /* Set up logging and give us a name: useful for debugging if the thread faults */
    zf_log_set_tag_prefix("server-1:");
    name_thread(seL4_CapInitThreadTCB, "server-1");

    /* get boot info */
    info = seL4_GetBootInfo();

    /* init simple */
    simple_default_init_bootinfo(&simple, info);

    /* print out bootinfo and other info about simple */
    simple_print(&simple);

    /* create an allocator */
    allocman = bootstrap_use_current_simple(&simple, ALLOCATOR_STATIC_POOL_SIZE,        allocator_mem_pool);
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

    /* create a new TCB */
    vka_object_t tcb_object = {0};
    error = vka_alloc_tcb(&vka, &tcb_object);
    ZF_LOGF_IFERR(error, "Failed to allocate new TCB.\n"
                  "\tVKA given sufficient bootstrap memory?");

    /*
     * create and map an ipc buffer:
     */

    /* Get a frame cap for the ipc buffer */
    vka_object_t ipc_frame_object;
    error = vka_alloc_frame(&vka, IPCBUF_FRAME_SIZE_BITS, &ipc_frame_object);
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

    seL4_Word ipc_buffer_vaddr = IPCBUF_VADDR;

    /* Try to map the frame the first time  */
    error = seL4_ARCH_Page_Map(ipc_frame_object.cptr, pd_cap, ipc_buffer_vaddr,
                               seL4_AllRights, seL4_ARCH_Default_VMAttributes);

    if (error != 0) {
        /* Create a page table */
        vka_object_t pt_object;
        error =  vka_alloc_page_table(&vka, &pt_object);
        ZF_LOGF_IFERR(error, "Failed to allocate new page table.\n");

        /* Map the page table */
        error = seL4_ARCH_PageTable_Map(pt_object.cptr, pd_cap,
                                        ipc_buffer_vaddr, seL4_ARCH_Default_VMAttributes);
        ZF_LOGF_IFERR(error, "Failed to map page table into VSpace.\n"
                      "\tWe are inserting a new page table into the top-level table.\n"
                      "\tPass a capability to the new page table, and not for example, the IPC buffer frame vaddr.\n")

        /* then map the frame in */
        error = seL4_ARCH_Page_Map(ipc_frame_object.cptr, pd_cap,
                                   ipc_buffer_vaddr, seL4_AllRights, seL4_ARCH_Default_VMAttributes);
        ZF_LOGF_IFERR(error, "Failed again to map the IPC buffer frame into the VSpace.\n"
                      "\t(It's not supposed to fail.)\n"
                      "\tPass a capability to the IPC buffer's physical frame.\n"
                      "\tRevisit the first seL4_ARCH_Page_Map call above and double-check your arguments.\n");
    }

    /* set the IPC buffer's virtual address in a field of the IPC buffer */
    seL4_IPCBuffer *ipcbuf = (seL4_IPCBuffer*)ipc_buffer_vaddr;
    ipcbuf->userData = ipc_buffer_vaddr;

    /* create an endpoint */
    error = vka_alloc_endpoint(&vka, &ep_object);
    ZF_LOGF_IFERR(error, "Failed to allocate new endpoint object.\n");

    /* make a badged copy of it in our cspace. This copy will be used to send
     * an IPC message to the original cap */

    error = vka_mint_object(&vka, &ep_object, &ep_cap_path, seL4_AllRights,
                            seL4_CapData_Badge_new(EP_BADGE));
    ZF_LOGF_IFERR(error, "Failed to mint new badged copy of IPC endpoint.\n"
                  "\tseL4_Mint is the backend for vka_mint_object.\n"
                  "\tseL4_Mint is simply being used here to create a badged copy of the same IPC endpoint.\n"
                  "\tThink of a badge in this case as an IPC context cookie.\n");

    /* TODO 1: initialise the new TCB */
    /* hint 1: seL4_TCB_Configure()
       There are a couple of new arguments, namely sched_context,
       prio and timeout_fault_ep */
    /* hint 2: seL4_Prio_new() */
    /* hint 3: You can initialise a TCB with a null scheduling context */

    /* TODO 2: Create a cap to store a sched context */
    /* hint 1: vka_alloc_sched_context */

    /* TODO 3: initialise a scheduling context */
    /* hint 1: seL4_SchedControl_Configure */
    /* hint 2: simple_get_sched_ctrl */


    /* TODO 4: Bind the scheduling context */
    /* hint 1: seL4_SchedContext_Bind */
    /* You now have an active server, try running it */



    /* give the new thread a name */
    name_thread(tcb_object.cptr, "server-1: server");

    /* set start up registers for the new thread */
    seL4_UserContext regs = {0};
    size_t regs_size = sizeof(seL4_UserContext) / sizeof(seL4_Word);

    /* set instruction pointer where the thread shoud start running */
    sel4utils_set_instruction_pointer(&regs, (seL4_Word)server);

    /* check that stack is aligned correctly */
    const int stack_alignment_requirement = sizeof(seL4_Word) * 2;
    uintptr_t server_stack_top = (uintptr_t)server_stack + sizeof(server_stack);

    ZF_LOGF_IF(server_stack_top % (stack_alignment_requirement) != 0,
               "Stack top isn't aligned correctly to a %dB boundary.\n"
               "\tDouble check to ensure you're not trampling.",
               stack_alignment_requirement);

    /* set stack pointer for the new thread. remember the stack grows down */
    sel4utils_set_stack_pointer(&regs, server_stack_top);

    /* actually write the TCB registers. */
    error = seL4_TCB_WriteRegisters(tcb_object.cptr, 0, 0, regs_size, &regs);
    ZF_LOGF_IFERR(error, "Failed to write the new thread's register set.\n"
                  "\tDid you write the correct number of registers? See arg4.\n");

    /* start the new thread running */
    error = seL4_TCB_Resume(tcb_object.cptr);
    ZF_LOGF_IFERR(error, "Failed to start new thread.\n");

    /* we are done, say hello */
    printf("main: hello world\n");

    /* TODO 6: Get signal from server saying it initialised */
    /* hint 1: seL4_Wait */

    /* TODO 7: Unbind the server's scheduling context */
    /* hint 1: seL4_SchedContext_Unbind */

    /* Set the data to send to the server */
    seL4_MessageInfo_t msg = seL4_MessageInfo_new(0, 0, 0, 1);

    /* Call the server, donating our scheduling context, if it's passive */
    printf("main: sending 1st request to server\n");
    seL4_SetMR(0, MSG_DATA);
    seL4_Call(ep_cap_path.capPtr, msg);
    printf("main: sending 2nd request to server\n");
    seL4_SetMR(0, MSG_DATA + 1);
    seL4_Call(ep_cap_path.capPtr, msg);
    printf("main: sending 3rd request to server\n");
    seL4_SetMR(0, MSG_DATA + 2);
    seL4_Call(ep_cap_path.capPtr, msg);
    printf("main: finished\n");
    seL4_Wait(ep_cap_path.capPtr, NULL);
    return 0;
}
