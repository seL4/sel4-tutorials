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
 * Part X: IPC between 2 threads
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
//#include <vka/vka.h>

#include <allocman/allocman.h>
#include <allocman/bootstrap.h>
#include <allocman/vka.h>

#include <vspace/vspace.h>

#include <sel4utils/vspace.h>
#include <sel4utils/mapping.h>

#define IPCBUF_FRAME_SIZE_BITS 12
#define IPCBUF_VADDR 0x7000000

#define EP_BADGE 0x61 // arbitrary (but unique) number

#define MSG_DATA 0x6161 // arbitrary data to send

seL4_BootInfo *info;
simple_t simple;
vka_t vka;
allocman_t *allocman;

vka_object_t ep_object;
cspacepath_t ep_cap_path;

/* static memory for the allocator to bootstrap with */
#define ALLOCATOR_STATIC_POOL_SIZE ((1 << seL4_PageBits) * 10)
UNUSED static char allocator_mem_pool[ALLOCATOR_STATIC_POOL_SIZE];

#define THREAD_2_STACK_SIZE 512
static uint64_t thread_2_stack[THREAD_2_STACK_SIZE];

void abort(void) {
    while (1);
}

void __arch_putchar(int c) {
#ifdef CONFIG_DEBUG_BUILD
    seL4_DebugPutChar(c);
#endif
}

void thread_2(void) {
    UNUSED seL4_Word sender_badge;
    UNUSED seL4_MessageInfo_t tag;
    UNUSED seL4_Word msg;

    printf("thread_2: hallo wereld\n");

    tag = seL4_Wait(ep_object.cptr, &sender_badge);
    assert(sender_badge == EP_BADGE);

    assert(seL4_MessageInfo_get_length(tag) == 1);
    msg = seL4_GetMR(0); 
    printf("thread_2: got a message %#x from %#x\n", msg, sender_badge);

    seL4_SetMR(0, ~msg);
    seL4_ReplyWait(ep_object.cptr, tag, &sender_badge);

    //while (1);
}

int main(void)
{
    UNUSED int error;
    UNUSED void *ipcbuf_addr;
    UNUSED seL4_CPtr ipc_frame;

#ifdef SEL4_DEBUG_KERNEL
    seL4_DebugNameThread(seL4_CapInitThreadTCB, "hello-libs-2");
#endif

    /* get boot info */
    info = seL4_GetBootInfo();

    printf("thread_2: %p\n", &thread_2);

    /* init simple */
    simple_default_init_bootinfo(&simple, info);

    /* Print bootinfo and other info about simple */
    simple_print(&simple);

    /* create an allocator */
    allocman = bootstrap_use_current_simple(&simple, ALLOCATOR_STATIC_POOL_SIZE, allocator_mem_pool);
    assert(allocman);

    /* create a vka (interface for interacting with the underlying allocator) */
    allocman_make_vka(&vka, allocman);

    /* get our cspace */
    seL4_CPtr cspace_cap;
    cspace_cap = simple_get_cnode(&simple);

    /* get our vspace root */
    seL4_CPtr pd_cap;
    pd_cap = simple_get_pd(&simple);

    /* create a TCB */
    vka_object_t tcb_object;
    tcb_object.cptr = 0; // compiler complains otherwise!
    error = vka_alloc_tcb(&vka, &tcb_object);
    assert(error == 0);

    /* create and map an ipc buffer */
    seL4_Word ipc_buffer_vaddr;
    ipc_buffer_vaddr = IPCBUF_VADDR;

    // get a frame cap
    vka_object_t ipc_frame_object;
    error = vka_alloc_frame(&vka, IPCBUF_FRAME_SIZE_BITS, &ipc_frame_object);
    assert(error == 0);

    // map it
    error = seL4_ARCH_Page_Map(ipc_frame_object.cptr, pd_cap, ipc_buffer_vaddr,
                               seL4_AllRights, seL4_ARCH_Default_VMAttributes);
    if (error != 0) {
        vka_object_t pt_object;
        error =  vka_alloc_page_table(&vka, &pt_object);
        assert(error == 0);

    	error = seL4_ARCH_PageTable_Map(pt_object.cptr, pd_cap, 
                                        ipc_buffer_vaddr,
                                        seL4_ARCH_Default_VMAttributes);
        assert(error == 0);

        error = seL4_ARCH_Page_Map(ipc_frame_object.cptr, pd_cap, 
                                   ipc_buffer_vaddr,
                                   seL4_AllRights, 
                                   seL4_ARCH_Default_VMAttributes);
        assert(error == 0);
    }

    seL4_IPCBuffer *ipcbuf = (seL4_IPCBuffer*)ipc_buffer_vaddr;
    ipcbuf->userData = ipc_buffer_vaddr;

    /* Create an endpoint */
    
    // create the object
    error = vka_alloc_endpoint(&vka, &ep_object);
    assert(error == 0);

    // make a copy of it
    vka_mint_object(&vka, &ep_object, &ep_cap_path, seL4_AllRights, seL4_CapData_Badge_new(EP_BADGE));
 
    seL4_CapData_t cspace_data;
    cspace_data = seL4_CapData_Guard_new(0 /* guard bits */, 32 - info->initThreadCNodeSizeBits);

    /* initialise the TCB */
    error = seL4_TCB_Configure(tcb_object.cptr /* service */, 
                               seL4_CapNull /* fault ep */, 
                               seL4_MaxPrio /* priority */,
                               cspace_cap /* cspace root */, 
                               cspace_data /* cspace root data */, 
                               pd_cap /* vspace root */,
                               seL4_NilData /* vspace root data */, 
                               ipc_buffer_vaddr /* buffer */, 
                               ipc_frame_object.cptr /* buffer frame */);
    assert(error == 0);

#ifdef SEL4_DEBUG_KERNEL
    seL4_DebugNameThread(tcb_object.cptr, "hello-libs-3: thread_2");
#endif

    seL4_UserContext regs = {0};
    size_t regs_size = sizeof(seL4_UserContext) / sizeof(seL4_Word);
    sel4utils_set_instruction_pointer(&regs, (seL4_Word)thread_2);

    /* check that stack is aligned correctly */
    assert((seL4_Word) thread_2_stack % (sizeof(seL4_Word) * 2) == 0);

    sel4utils_set_stack_pointer(&regs, (seL4_Word)(thread_2_stack + sizeof(thread_2_stack)));

    regs.gs = IPCBUF_GDT_SELECTOR;

/*    seL4_TCB_WriteRegisters(seL4_TCB service, seL4_Bool resume_target, seL4_Uint8 arch_flags, seL4_Word count, seL4_UserContext *regs)
      count: number of registers to transfer. IP is first, SP is second.
*/
    error = seL4_TCB_WriteRegisters(tcb_object.cptr, 0, 0, regs_size, &regs);
    assert(error == 0);

    /* start it running */
    error = seL4_TCB_Resume(tcb_object.cptr);
    assert(error == 0);

    printf("main: hello world\n");
 
    seL4_Word msg;
    seL4_MessageInfo_t tag = seL4_MessageInfo_new(0, 0, 0, 1);
    seL4_SetMR(0, MSG_DATA);

    tag = seL4_Call(ep_cap_path.capPtr, tag);

    assert(seL4_MessageInfo_get_length(tag) == 1);
    msg = seL4_GetMR(0);
    assert(msg == ~MSG_DATA);

    printf("main: got a reply: %#x\n", msg);

    return 0;
}

