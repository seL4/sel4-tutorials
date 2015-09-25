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

/* variables shared with second thread */
vka_object_t ep_object;
cspacepath_t ep_cap_path;

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

    /* TODO: wait for a message to come in over the endpoint */
    /* hint 1: seL4_Wait() 
     * seL4_MessageInfo_t seL4_Wait(seL4_CPtr src, seL4_Word* sender)
     * @param src The capability to be invoked.
     * @param sender The badge of the endpoint capability that was invoked by the sender is written to this address.
     * @return A seL4_MessageInfo_t structure
     * https://github.com/seL4/seL4/blob/master/libsel4/arch_include/x86/sel4/arch/syscalls.h#L165
     * You can find out more about it in the API manual: http://sel4.systems/Info/Docs/seL4-manual.pdf 
     *
     * hint 2: seL4_MessageInfo_t is generated during build.
     * The type definition and generated field access functions are defined in a generated file: 
     * build/x86/pc99/libsel4/include/sel4/types_gen.h
     * It is generated from the following definition:
     * https://github.com/seL4/seL4/blob/master/libsel4/include/sel4/types.bf#L35
     * You can find out more about it in the API manual: http://sel4.systems/Info/Docs/seL4-manual.pdf     
     */

    /* TODO: make sure it is what we expected */
    /* hint 1: check the badge. is it EP_BADGE?
     * hint 2: we are expecting only 1 message register
     * hint 3: seL4_MessageInfo_get_length()
     * seL4_Uint32 CONST seL4_MessageInfo_get_length(seL4_MessageInfo_t seL4_MessageInfo) 
     * @param seL4_MessageInfo the seL4_MessageInfo_t to extract a field from
     * @return the number of message registers delivered
     * seL4_MessageInfo_get_length() is generated during build. It can be found in:
     * build/x86/pc99/libsel4/include/sel4/types_gen.h
     * It is generated from the following definition:
     * https://github.com/seL4/seL4/blob/master/libsel4/include/sel4/types.bf#L35      * 
     */ 

    /* TODO: get the message stored in the first message register */
    /* hint: seL4_GetMR() 
     * seL4_Word seL4_GetMR(int i)
     * @param i The message register to retreive
     * @return The message register value
     * https://github.com/seL4/seL4/blob/master/libsel4/arch_include/x86/sel4/arch/functions.h#L33
     * You can find out more about message registers in the API manual: http://sel4.systems/Info/Docs/seL4-manual.pdf
     */

    printf("thread_2: got a message %#x from %#x\n", msg, sender_badge);

    /* modify the message */
    msg = ~msg;

    /* TODO: copy the modified message back into the message register */
    /* hint: seL4_SetMR() 
     * void seL4_SetMR(int i, seL4_Word mr)
     * @param i The message register to write
     * @param mr The value of the message register
     * https://github.com/seL4/seL4/blob/master/libsel4/arch_include/x86/sel4/arch/functions.h#L41
     * You can find out more about message registers in the API manual: http://sel4.systems/Info/Docs/seL4-manual.pdf
     */

    /* TODO: send the message back */
    /* hint 1: seL4_ReplyWait()
     * seL4_MessageInfo_t seL4_ReplyWait(seL4_CPtr dest, seL4_MessageInfo_t msgInfo, seL4_Word *sender) 
     * @param dest The capability to be invoked.
     * @param msgInfo The messageinfo structure for the IPC.  This specifies information about the message to send (such as the number of message registers to send) as the Reply part.
     * @param sender The badge of the endpoint capability that was invoked by the sender is written to this address.  This is a result of the Wait part.
     * @return A seL4_MessageInfo_t structure.  This is a result of the Wait part.
     * https://github.com/seL4/seL4/blob/master/libsel4/arch_include/x86/sel4/arch/syscalls.h#L324
     * You can find out more about it in the API manual: http://sel4.systems/Info/Docs/seL4-manual.pdf 
     *
     * hint 2: seL4_MessageInfo_t is generated during build.
     * The type definition and generated field access functions are defined in a generated file: 
     * build/x86/pc99/libsel4/include/sel4/types_gen.h
     * It is generated from the following definition:
     * https://github.com/seL4/seL4/blob/master/libsel4/include/sel4/types.bf#L35
     * You can find out more about it in the API manual: http://sel4.systems/Info/Docs/seL4-manual.pdf     
     */
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

    /* TODO: get a frame cap for the ipc buffer */
    /* hint vka_alloc_frame() 
     * int vka_alloc_frame(vka_t *vka, uint32_t size_bits, vka_object_t *result)
     * @param vka Pointer to vka interface.
     * @param size_bits Frame size: 2^size_bits
     * @param result Structure for the Frame object.  This gets initialised.
     * @return 0 on success
     * https://github.com/seL4/libsel4vka/blob/master/include/vka/object.h#L147
     */
    vka_object_t ipc_frame_object;

    /*
     * map the frame into the vspace at ipc_buffer_vaddr.
     * To do this we first try to map it in to the root page directory.
     * If there is already a page table mapped in the appropriate slot in the
     * page diretory where we can insert this frame, then this will succeed.
     * Otherwise we first need to create a page table, and map it in to
     * the page directory, before we can map the frame in. */

    seL4_Word ipc_buffer_vaddr = IPCBUF_VADDR;

    /* TODO: try to map the frame the first time  */
    /* hint 1: seL4_ARCH_Page_Map()
     * The *ARCH* versions of seL4 sys calls are abstractions over the architecture provided by libsel4utils
     * this one is defined as:
     * #define seL4_ARCH_Page_Map seL4_IA32_Page_Map
     * in: https://github.com/seL4/libsel4utils/blob/master/include/sel4utils/mapping.h#L69
     * The signature for the underlying function is:
     * int seL4_IA32_Page_Map(seL4_IA32_Page service, seL4_IA32_PageDirectory pd, seL4_Word vaddr, seL4_CapRights rights, seL4_IA32_VMAttributes attr)
     * @param service Capability to the page to map.
     * @param pd Capability to the VSpace which will contain the mapping.
     * @param vaddr Virtual address to map the page into.
     * @param rights Rights for the mapping.
     * @param attr VM Attributes for the mapping.
     * @return 0 on success.
     *
     * Note: this function is generated during build.  It is generated from the following definition:
     * https://github.com/seL4/seL4/blob/master/libsel4/arch_include/x86/interfaces/sel4arch.xml#L52
     * You can find out more about it in the API manual: http://sel4.systems/Info/Docs/seL4-manual.pdf    
     *
     * hint 2: for the rights, use seL4_AllRights 
     * hint 3: for VM attributes use seL4_ARCH_Default_VMAttributes
     */


    if (error != 0) {
        /* TODO: create a page table */
        /* hint: vka_alloc_page_table()
	 * int vka_alloc_page_table(vka_t *vka, vka_object_t *result)
	 * @param vka Pointer to vka interface.
	 * @param result Structure for the PageTable object.  This gets initialised.
	 * @return 0 on success
         * https://github.com/seL4/libsel4vka/blob/master/include/vka/object.h#L178
         */
        vka_object_t pt_object;

        /* TODO: map the page table */
        /* hint 1: seL4_ARCH_PageTable_Map()
	 * The *ARCH* versions of seL4 sys calls are abstractions over the architecture provided by libsel4utils
	 * this one is defined as:
	 * #define seL4_ARCH_PageTable_Map seL4_IA32_PageTable_Map
	 * in: https://github.com/seL4/libsel4utils/blob/master/include/sel4utils/mapping.h#L73
	 * The signature for the underlying function is:
	 * int seL4_IA32_PageTable_Map(seL4_IA32_PageTable service, seL4_IA32_PageDirectory pd, seL4_Word vaddr, seL4_IA32_VMAttributes attr)
	 * @param service Capability to the page table to map.
	 * @param pd Capability to the VSpace which will contain the mapping.
	 * @param vaddr Virtual address to map the page table into.
	 * @param rights Rights for the mapping.
	 * @param attr VM Attributes for the mapping.
	 * @return 0 on success.
	 *
	 * Note: this function is generated during build.  It is generated from the following definition:
	 * https://github.com/seL4/seL4/blob/master/libsel4/arch_include/x86/interfaces/sel4arch.xml#L37
	 * You can find out more about it in the API manual: http://sel4.systems/Info/Docs/seL4-manual.pdf    
	 * 
         * hint 2: for VM attributes use seL4_ARCH_Default_VMAttributes
         */


        /* TODO: then map the frame in */
        /* hint 1: use seL4_ARCH_Page_Map() as above
         * hint 2: for the rights, use seL4_AllRights 
         * hint 3: for VM attributes use seL4_ARCH_Default_VMAttributes
         */


    }

    /* set the IPC buffer's virtual address in a field of the IPC buffer */
    seL4_IPCBuffer *ipcbuf = (seL4_IPCBuffer*)ipc_buffer_vaddr;
    ipcbuf->userData = ipc_buffer_vaddr;

    /* TODO: create an endpoint */
    /* hint: vka_alloc_endpoint() 
     * int vka_alloc_endpoint(vka_t *vka, vka_object_t *result)
     * @param vka Pointer to vka interface.
     * @param result Structure for the Endpoint object.  This gets initialised.
     * @return 0 on success
     * https://github.com/seL4/libsel4vka/blob/master/include/vka/object.h#L94
     */


    /* TODO: make a badged copy of it in our cspace. This copy will be used to send 
     * an IPC message to the original cap */
    /* hint 1: vka_mint_object()
     * int vka_mint_object(vka_t *vka, vka_object_t *object, cspacepath_t *result, seL4_CapRights rights, seL4_CapData_t badge) 
     * @param[in] vka The allocator for the cspace.
     * @param[in] object Target object for cap minting.
     * @param[out] result Allocated cspacepath.
     * @param[in] rights The rights for the minted cap.
     * @param[in] badge The badge for the minted cap. 
     * @return 0 on success
     * 
     * https://github.com/seL4/libsel4vka/blob/master/include/vka/object_capops.h#L41
     *
     * hint 2: for the rights, use seL4_AllRights
     * hint 3: for the badge use seL4_CapData_Badge_new()
     * seL4_CapData_t CONST seL4_CapData_Badge_new(seL4_Uint32 Badge)
     * @param[in] Badge The badge number to use
     * @return A CapData structure containing the desired badge info
     * 
     * seL4_CapData_t is generated during build.
     * The type definition and generated field access functions are defined in a generated file: 
     * build/x86/pc99/libsel4/include/sel4/types_gen.h
     * It is generated from the following definition:
     * https://github.com/seL4/seL4/blob/master/libsel4/include/sel4/types.bf#L30
     * You can find out more about it in the API manual: http://sel4.systems/Info/Docs/seL4-manual.pdf     
     * 
     * hint 4: for the badge use EP_BADGE
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

    seL4_Word msg;
    seL4_MessageInfo_t tag;

    /* TODO: set the data to send. We send it in the first message register */
    /* hint 1: seL4_MessageInfo_new()
     * seL4_MessageInfo_t CONST seL4_MessageInfo_new(seL4_Uint32 label, seL4_Uint32 capsUnwrapped, seL4_Uint32 extraCaps, seL4_Uint32 length) 
     * @param label The value of the label field
     * @param capsUnwrapped The value of the capsUnwrapped field
     * @param extraCaps The value of the extraCaps field
     * @param length The number of message registers to send
     * @return The seL4_MessageInfo_t containing the given values.
     *
     * seL4_MessageInfo_new() is generated during build. It can be found in:
     * build/x86/pc99/libsel4/include/sel4/types_gen.h
     * It is generated from the following definition:
     * https://github.com/seL4/seL4/blob/master/libsel4/include/sel4/types.bf#L35
     *
     * hint 2: use 0 for the first 3 fields.
     * hint 3: send only 1 message register of data
     *
     * hint 4: seL4_SetMR()
     * void seL4_SetMR(int i, seL4_Word mr)
     * @param i The message register to write
     * @param mr The value of the message register
     * https://github.com/seL4/seL4/blob/master/libsel4/arch_include/x86/sel4/arch/functions.h#L41
     * You can find out more about message registers in the API manual: http://sel4.systems/Info/Docs/seL4-manual.pdf
     *
     * hint 5: send MSG_DATA
     */


    /* TODO: send and wait for a reply. */
    /* hint: seL4_Call() 
     * seL4_MessageInfo_t seL4_Call(seL4_CPtr dest, seL4_MessageInfo_t msgInfo)
     * @param dest The capability to be invoked.
     * @param msgInfo The messageinfo structure for the IPC.  This specifies information about the message to send (such as the number of message registers to send).
     * @return A seL4_MessageInfo_t structure.  This is information about the repy message.
     * https://github.com/seL4/seL4/blob/master/libsel4/arch_include/x86/sel4/arch/syscalls.h#L242
     * You can find out more about it in the API manual: http://sel4.systems/Info/Docs/seL4-manual.pdf 
     *
     * hint 2: seL4_MessageInfo_t is generated during build.
     * The type definition and generated field access functions are defined in a generated file: 
     * build/x86/pc99/libsel4/include/sel4/types_gen.h
     * It is generated from the following definition:
     * https://github.com/seL4/seL4/blob/master/libsel4/include/sel4/types.bf#L35
     * You can find out more about it in the API manual: http://sel4.systems/Info/Docs/seL4-manual.pdf     
     */


    /* TODO: get the reply message */
    /* hint: seL4_GetMR()
     * seL4_Word seL4_GetMR(int i)
     * @param i The message register to retreive
     * @return The message register value
     * https://github.com/seL4/seL4/blob/master/libsel4/arch_include/x86/sel4/arch/functions.h#L33
     * You can find out more about message registers in the API manual: http://sel4.systems/Info/Docs/seL4-manual.pdf
     */


    /* check that we got the expected repy */
    assert(seL4_MessageInfo_get_length(tag) == 1);
    assert(msg == ~MSG_DATA);

    printf("main: got a reply: %#x\n", msg);

    return 0;
}

