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

#define SEL4_ZF_LOG_ON

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

#include <utils/zf_log.h>

/* constants */
#define EP_BADGE 0x61 // arbitrary (but unique) number for a badge
#define MSG_DATA 0x6161 // arbitrary data to send

#define APP_PRIORITY seL4_MaxPrio
#define APP_IMAGE_NAME "hello-4-app"

/* global environment variables */
seL4_BootInfo *info;
simple_t simple;
vka_t vka;
allocman_t *allocman;
vspace_t vspace;

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
    name_thread(seL4_CapInitThreadTCB, "hello-4");

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

   /* TODO 1: create a vspace object to manage our vspace */
   /* hint 1: sel4utils_bootstrap_vspace_with_bootinfo_leaky() 
    * int sel4utils_bootstrap_vspace_with_bootinfo_leaky(vspace_t *vspace, sel4utils_alloc_data_t *data, seL4_CPtr page_directory, vka_t *vka, seL4_BootInfo *info)
    * @param vspace Uninitialised vspace struct to populate.
    * @param data Uninitialised vspace data struct to populate.
    * @param page_directory Page directory for the new vspace.
    * @param vka Initialised vka that this virtual memory allocator will use to allocate pages and pagetables. This allocator will never invoke free.
    * @param info seL4 boot info
    * @return 0 on succes.
    * https://github.com/seL4/seL4_libs/blob/3.0.x-compatible/libsel4utils/include/sel4utils/vspace.h#L214
    * https://github.com/seL4/seL4_libs/blob/3.0.x-compatible/libsel4utils/include/sel4utils/vspace.h#L172
    */

    /* fill the allocator with virtual memory */
    void *vaddr;
    UNUSED reservation_t virtual_reservation;
    virtual_reservation = vspace_reserve_range(&vspace,
        ALLOCATOR_VIRTUAL_POOL_SIZE, seL4_AllRights, 1, &vaddr);
    assert(virtual_reservation.res);
    bootstrap_configure_virtual_pool(allocman, vaddr,
        ALLOCATOR_VIRTUAL_POOL_SIZE, simple_get_pd(&simple));

   /* TODO 2: use sel4utils to make a new process */
   /* hint 1: sel4utils_configure_process() 
    * int sel4utils_configure_process(sel4utils_process_t *process, vka_t *vka, vspace_t *vspace, uint8_t priority, char *image_name);
    * @param process Uninitialised process struct.
    * @param vka Allocator to use to allocate objects.
    * @param vspace Vspace allocator for the current vspace.
    * @param priority Priority to configure the process to run as.
    * @param image_name Name of the elf image to load from the cpio archive.
    * @return 0 on success, -1 on error.
    * https://github.com/seL4/seL4_libs/blob/3.0.x-compatible/libsel4utils/include/sel4utils/process.h#L165
    *
    * hint 2: priority is in APP_PRIORITY and can be 0 to seL4_MaxPrio
    * hint 3: the elf image name is in APP_IMAGE_NAME
    */

    /* TODO 3: give the new process's thread a name */

    /* create an endpoint */
    vka_object_t ep_object = {0};
    error = vka_alloc_endpoint(&vka, &ep_object);
    assert(error == 0);

    /*
     * make a badged endpoint in the new process's cspace.  This copy 
     * will be used to send an IPC to the original cap
     */

    /* TODO 4: make a cspacepath for the new endpoint cap */
    /* hint 1: vka_cspace_make_path()
     * void vka_cspace_make_path(vka_t *vka, seL4_CPtr slot, cspacepath_t *res)
     * @param vka Vka interface to use for allocation of objects.
     * @param slot A cslot allocated by the cspace alloc function
     * @param res Pointer to a cspacepath struct to fill out
     * https://github.com/seL4/seL4_libs/blob/3.0.x-compatible/libsel4vka/include/vka/vka.h#L122
     *
     * hint 2: use the cslot of the endpoint allocated above
     */

    /* TODO 5: copy the endpont cap and add a badge to the new cap */
    /* hint 1: sel4utils_mint_cap_to_process()
     * seL4_CPtr sel4utils_mint_cap_to_process(sel4utils_process_t *process, cspacepath_t src, seL4_CapRights rights, seL4_CapData_t data)
     * @param process Process to copy the cap to
     * @param src Path in the current cspace to copy the cap from
     * @param rights The rights of the new cap
     * @param data Extra data for the new cap (e.g., the badge)
     * @return 0 on failure, otherwise the slot in the processes cspace.
     * https://github.com/seL4/seL4_libs/blob/3.0.x-compatible/libsel4utils/include/sel4utils/process.h#L227
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
     * https://github.com/seL4/seL4/blob/3.0.0/libsel4/include/sel4/types_32.bf#L30
     * You can find out more about it in the API manual: http://sel4.systems/Info/Docs/seL4-manual-3.0.0.pdf
     * 
     * hint 4: for the badge value use EP_BADGE
     */

    /* TODO 6: spawn the process */
    /* hint 1: sel4utils_spawn_process_v() 
     * int sel4utils_spawn_process(sel4utils_process_t *process, vka_t *vka, vspace_t *vspace, int argc, char *argv[], int resume)
     * @param process Initialised sel4utils process struct.
     * @param vka Vka interface to use for allocation of frames.
     * @param vspace The current vspace.
     * @param argc The number of arguments.
     * @param argv A pointer to an array of strings in the current vspace.
     * @param resume 1 to start the process, 0 to leave suspended.
     * @return 0 on success, -1 on error.
     * https://github.com/seL4/seL4_libs/blob/3.0.x-compatible/libsel4utils/include/sel4utils/process.h#L161
     */

    /* we are done, say hello */
    printf("main: hello world\n");

    /*
     * now wait for a message from the new process, then send a reply back
     */

    seL4_Word sender_badge;
    seL4_MessageInfo_t tag;
    seL4_Word msg;

    /* TODO 7: wait for a message */
    /* hint 1: seL4_Recv() 
     * seL4_MessageInfo_t seL4_Recv(seL4_CPtr src, seL4_Word* sender)
     * @param src The capability to be invoked.
     * @param sender The badge of the endpoint capability that was invoked by the sender is written to this address.
     * @return A seL4_MessageInfo_t structure
     * https://github.com/seL4/seL4/blob/3.0.0/libsel4/sel4_arch_include/ia32/sel4/sel4_arch/syscalls.h#L164
     * You can find out more about it in the API manual: http://sel4.systems/Info/Docs/seL4-manual-3.0.0.pdf
     *
     * hint 2: seL4_MessageInfo_t is generated during build.
     * The type definition and generated field access functions are defined in a generated file: 
     * build/x86/pc99/libsel4/include/sel4/types_gen.h
     * It is generated from the following definition:
     * https://github.com/seL4/seL4/blob/3.0.0/libsel4/include/sel4/shared_types_32.bf#L15
     * You can find out more about it in the API manual: http://sel4.systems/Info/Docs/seL4-manual-3.0.0.pdf
     *
     * hint 3: use the original endpoint cap 
     */

    /* make sure it is what we expected */
    assert(sender_badge == EP_BADGE);
    assert(seL4_MessageInfo_get_length(tag) == 1);

    /* get the message stored in the first message register */
    msg = seL4_GetMR(0);
    printf("main: got a message %#x from %#x\n", msg, sender_badge);

    /* modify the message */
    seL4_SetMR(0, ~msg);

    /* TODO 8: send the modified message back */
    /* hint 1: seL4_ReplyRecv()
     * seL4_MessageInfo_t seL4_ReplyRecv(seL4_CPtr dest, seL4_MessageInfo_t msgInfo, seL4_Word *sender) 
     * @param dest The capability to be invoked.
     * @param msgInfo The messageinfo structure for the IPC.  This specifies information about the message to send (such as the number of message registers to send) as the Reply part.
     * @param sender The badge of the endpoint capability that was invoked by the sender is written to this address. This is a result of the Wait part.
     * @return A seL4_MessageInfo_t structure.  This is a result of the Wait part.
     * https://github.com/seL4/seL4/blob/3.0.0/libsel4/sel4_arch_include/ia32/sel4/sel4_arch/syscalls.h#L359
     * You can find out more about it in the API manual: http://sel4.systems/Info/Docs/seL4-manual-3.0.0.pdf
     *
     * hint 2: seL4_MessageInfo_t is generated during build.
     * The type definition and generated field access functions are defined in a generated file: 
     * build/x86/pc99/libsel4/include/sel4/types_gen.h
     * It is generated from the following definition:
     * https://github.com/seL4/seL4/blob/3.0.0/libsel4/include/sel4/shared_types_32.bf#L15
     * You can find out more about it in the API manual: http://sel4.systems/Info/Docs/seL4-manual-3.0.0.pdf
     *
     * hint 3: use the endpoint cap that you used for Wait
     */

    return 0;
}

