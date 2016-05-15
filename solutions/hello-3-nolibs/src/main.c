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
#include <stdint.h>

#include <sel4/sel4.h>
#include <sel4/types_gen.h>
#include <sel4debug/debug.h>

#include <utils/zf_log.h>
#include <sel4utils/sel4_zf_logif.h>

/* constants */
#define IPCBUF_FRAME_SIZE_BITS 12 // use a 4K frame for the IPC buffer
#define IPCBUF_VADDR 0x7000000 // arbitrary (but free) address for IPC buffer

#define EP_BADGE 0x61 // arbitrary (but unique) number for a badge
#define MSG_DATA 0x6161 // arbitrary data to send

/* global environment variables */
seL4_BootInfo *info;

seL4_CPtr ep_cap;
seL4_CPtr badged_ep_cap;

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

    /* wait for a message to come in over the endpoint */
    tag = seL4_Recv(ep_cap, &sender_badge);

    /* make sure it is what we expected */
    ZF_LOGF_IF(sender_badge != EP_BADGE,
        "Badge on the endpoint was not what was expected.\n");

    ZF_LOGF_IF(seL4_MessageInfo_get_length(tag) != 1,
        "Length of the data send from root thread was not what was expected.\n"
        "\tHow many registers did you set with seL4_SetMR, within the root thread?\n");


    /* get the message stored in the first message register */
    msg = seL4_GetMR(0);
    printf("thread_2: got a message %#x from %#x\n", msg, sender_badge);

    /* modify the message and send it back */
    seL4_SetMR(0, ~msg);
    seL4_ReplyRecv(ep_cap, tag, &sender_badge);
}


/* returns a cap to an untyped with a size at least size_bytes, or -1 if none exists */
seL4_CPtr get_untyped(seL4_BootInfo *info, int size_bytes) {

    for (int i = info->untyped.start, idx = 0; i < info->untyped.end; ++i, ++idx) {
        
        if (1<<info->untypedSizeBitsList[idx] >= size_bytes) {
            return i;
        }
    }

    return -1;
}

/* Retypes an untyped object to the specified object of specified size, storing a cap to that object
 * in the specified slot of the cspace whose root is root_cnode. This requires that the root_cnode
 * argument is also the root cnode of the cspace of the calling thread.
 */
int untyped_retype_root(seL4_CPtr untyped, seL4_ObjectType type, int size_bits, seL4_CPtr root_cnode, seL4_CPtr slot) {
    return seL4_Untyped_Retype(untyped /* untyped cap */,
                                type /* type */, 
                                size_bits /* size */, 
                                root_cnode /* root cnode cap */,
                                root_cnode /* destination cspace */,
                                32 /* depth */,
                                slot /* offset */,
                                1 /* num objects */);
}

int main(void)
{
    int error;

    /* Set up logging and give us a name: useful for debugging if the thread faults */
    zf_log_set_tag_prefix("hello-3:");
    name_thread(seL4_CapInitThreadTCB, "hello-3");

    /* get boot info */
    info = seL4_GetBootInfo();

    /* print out bootinfo */
    debug_print_bootinfo(info);

    /* get our cspace root cnode */
    seL4_CPtr cspace_cap;
    cspace_cap = seL4_CapInitThreadCNode;

    /* get our vspace root page directory */
    seL4_CPtr pd_cap;
    pd_cap = seL4_CapInitThreadPD;

    /* TODO 1: Find free cap slots for the caps to the:
     *  - tcb
     *  - ipc frame
     *  - endpoint
     *  - badged endpoint
     *  - page table
     * hint: The bootinfo struct contains a range of free cap slot indices.
     */

    /* decide on slots to use based on what is free */
    seL4_CPtr tcb_cap = info->empty.start;
    seL4_CPtr ipc_frame_cap = info->empty.start + 1;
    ep_cap = info->empty.start + 2;
    badged_ep_cap = info->empty.start + 3;
    seL4_CPtr page_table_cap = info->empty.start + 4;

    /* get an untyped to retype into all the objects we will need */
    seL4_CPtr untyped;

    /* TODO 2: Obtain a cap to an untyped which is large enough to contain:
     *  - tcb
     *  - ipc frame
     *  - endpoint
     *  - badged endpoint
     *  - page table
     *
     * hint 1: determine the size of each object
     *         (look in libs/libsel4/arch_include/x86/sel4/arch/types.h)
     * hint 2: an array of untyped caps, and a corresponding array of untyped sizes
     *         can be found in the bootinfo struct
     * hint 3: a single untyped cap can be retyped multiple times. Each time, an appropriately sized chunk
     *         of the object is "chipped off". For simplicity, find a cap to an untyped which is large enough
     *         to contain all required objects.
     */
    untyped = get_untyped(info, (1<<seL4_TCBBits) +
                                (1<<seL4_PageBits) +
                                (1<<seL4_EndpointBits) +
                                (1<<seL4_PageTableBits));
    ZF_LOGF_IF(untyped == -1, "Failed to find an untyped which could hold %d bytes.\n",
        (1<<seL4_TCBBits) +
        (1<<seL4_PageBits) +
        (1<<seL4_EndpointBits) +
        (1<<seL4_PageTableBits));
    
    /* TODO 3: Using the untyped, create the required objects, storing their caps in the roottask's root cnode.
     *
     * hint 1: int seL4_Untyped_Retype(seL4_Untyped service, int type, int size_bits, seL4_CNode root, int node_index, int node_depth, int node_offset, int num_objects)
     * hint 2: use a depth of 32
     * hint 3: use cspace_cap for the root cnode AND the cnode_index
     */
   /* create required objects */
    error = untyped_retype_root(untyped, seL4_TCBObject, seL4_TCBBits, cspace_cap, tcb_cap);
    ZF_LOGF_IFERR(error, "Failed to retype our chosen untyped into a TCB child object.\n");
    error = untyped_retype_root(untyped, seL4_X86_4K, seL4_PageBits, cspace_cap, ipc_frame_cap);
    ZF_LOGF_IFERR(error, "Failed to retype our chosen untyped into a page object.\n");
    error = untyped_retype_root(untyped, seL4_EndpointObject, seL4_EndpointBits, cspace_cap, ep_cap);
    ZF_LOGF_IFERR(error, "Failed to retype our chosen untyped into an Endpoint child object.\n");

    /*
     * map the frame into the vspace at ipc_buffer_vaddr.
     * To do this we first try to map it in to the root page directory.
     * If there is already a page table mapped in the appropriate slot in the
     * page directory where we can insert this frame, then this will succeed.
     * Otherwise we first need to create a page table, and map it in to
     * the page directory, before we can map the frame in.
     *
	 * It is normal for this function call to fail. Proceed to the next step
	 * where you'll be led to allocate a new empty page table.
	 */
    seL4_Word ipc_buffer_vaddr;
    ipc_buffer_vaddr = IPCBUF_VADDR;
    error = seL4_X86_Page_Map(ipc_frame_cap, pd_cap, ipc_buffer_vaddr,
        seL4_AllRights, seL4_X86_Default_VMAttributes);
    if (error != 0) {

        /* TODO 4: Retype the untyped into page table (if this was done in TODO 3, ignore this). */

        /* create and map a page table */
        error = untyped_retype_root(untyped, seL4_X86_PageTableObject, seL4_PageTableBits, cspace_cap, page_table_cap);
        ZF_LOGF_IFERR(error, "Failed to retype an object into a page table.\n"
            "Re-examine your arguments -- check the solution files if you're unable to get it.\n");
        
        error = seL4_X86_PageTable_Map(page_table_cap, pd_cap,
            ipc_buffer_vaddr, seL4_X86_Default_VMAttributes);
        ZF_LOGF_IFERR(error, "Failed to map page table into VSpace.\n"
            "\tWe are inserting a new page table into the top-level table.\n"
            "\tPass a capability to the new page table, and not for example, the IPC buffer frame vaddr.\n");

        /* then map the frame in */
        error = seL4_X86_Page_Map(ipc_frame_cap, pd_cap,
            ipc_buffer_vaddr, seL4_AllRights, seL4_X86_Default_VMAttributes);
        ZF_LOGF_IFERR(error, "Failed again to map the IPC buffer frame into the VSpace.\n"
            "\tPass a capability to the IPC buffer's physical frame.\n"
            "\tRevisit the first seL4_ARCH_Page_Map call above and double-check your arguments.\n");
    }

    /* set the IPC buffer's virtual address in a field of the IPC buffer */
    seL4_IPCBuffer *ipcbuf = (seL4_IPCBuffer*)ipc_buffer_vaddr;
    ipcbuf->userData = ipc_buffer_vaddr;

    /* TODO 5: Mint a copy of the endpoint cap into our cspace, using the badge EP_BADGE
     * hint: int seL4_CNode_Mint(seL4_CNode service, seL4_Word dest_index, seL4_Uint8 dest_depth, seL4_CNode src_root, seL4_Word src_index, seL4_Uint8 src_depth, seL4_CapRights rights, seL4_CapData_t badge);
     */

    /* create a copy of the endpoint cap with a badge (to use for sending) */
    seL4_CNode_Mint(cspace_cap, badged_ep_cap, 32, cspace_cap, ep_cap, 32,
        seL4_AllRights, seL4_CapData_Badge_new(EP_BADGE));


    /* initialise the new TCB */
    error = seL4_TCB_Configure(tcb_cap, seL4_CapNull, seL4_MaxPrio,
        cspace_cap, seL4_NilData, pd_cap, seL4_NilData,
        ipc_buffer_vaddr, ipc_frame_cap);
    ZF_LOGF_IFERR(error, "Failed to configure the new TCB object.\n"
        "\tWe're running the new thread with the root thread's CSpace.\n"
        "\tWe're running the new thread in the root thread's VSpace.\n");

    /* give the new thread a name */
    name_thread(tcb_cap, "hello-3: thread_2");

    /* set start up registers for the new thread */
    size_t regs_size = sizeof(seL4_UserContext) / sizeof(seL4_Word);

    /* check that stack is aligned correctly */
    const int stack_alignment_requirement = sizeof(seL4_Word) * 2;
    uintptr_t thread_2_stack_top = (uintptr_t)thread_2_stack + sizeof(thread_2_stack);
    ZF_LOGF_IF(thread_2_stack_top % (stack_alignment_requirement) != 0,
        "Stack top isn't aligned correctly to a %dB boundary.\n"
        "\tDouble check to ensure you're not trampling.",
        stack_alignment_requirement);

    /* set instruction pointer, stack pointer and gs register (used for thread local storage) */
    seL4_UserContext regs = {
        .eip = (seL4_Word)thread_2,
        .esp = (seL4_Word)thread_2_stack_top,
        .gs = IPCBUF_GDT_SELECTOR
    };

    /* actually write the TCB registers. */
    error = seL4_TCB_WriteRegisters(tcb_cap, 0, 0, regs_size, &regs);
    ZF_LOGF_IFERR(error, "Failed to write the new thread's register set.\n"
        "\tDid you write the correct number of registers? See arg4.\n");

    /* start the new thread running */
    error = seL4_TCB_Resume(tcb_cap);
    ZF_LOGF_IFERR(error, "Failed to start new thread.\n");

    /* we are done, say hello */
    printf("main: hello world\n");

    /*
     * now send a message to the new thread, and wait for a reply
     */

    seL4_Word msg;
    seL4_MessageInfo_t tag;

    /* set the data to send. We send it in the first message register */
    tag = seL4_MessageInfo_new(0, 0, 0, 1);
    seL4_SetMR(0, MSG_DATA);

    /* send and wait for a reply */
    tag = seL4_Call(badged_ep_cap, tag);

    /* check that we got the expected repy */
    ZF_LOGF_IF(seL4_MessageInfo_get_length(tag) != 1,
        "Response data from thread_2 was not the length expected.\n"
        "\tHow many registers did you set with seL4_SetMR within thread_2?\n");

    msg = seL4_GetMR(0);
    ZF_LOGF_IF(msg != ~MSG_DATA,
        "Response data from thread_2's content was not what was expected.\n");

    printf("main: got a reply: %#x\n", msg);

    return 0;
}
