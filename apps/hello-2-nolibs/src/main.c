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
 * seL4 tutorial part 2: create and run a new thread
 */

/* Include Kconfig variables. */
#include <autoconf.h>

#include <stdio.h>
#include <assert.h>
#include <stdint.h>

#include <sel4/sel4.h>

/* global environment variables */
seL4_BootInfo *info;

#define NUM_CNODE_SLOTS_BITS 4

/* stack for the new thread */
#define THREAD_2_STACK_SIZE 512
static uint64_t thread_2_stack[THREAD_2_STACK_SIZE];

/* convenience function */
extern void name_thread(seL4_CPtr tcb, char *name);

/* returns a cap to an untyped with a size at least size_bytes, or -1 if none exists */
seL4_CPtr get_untyped(seL4_BootInfo *info, int size_bytes) {

    for (int i = info->untyped.start, idx = 0; i < info->untyped.end; ++i, ++idx) {
        if (1<<info->untypedSizeBitsList[idx] >= size_bytes) {
            return i;
        }
    }

    return -1;
}

/* function to run in the new thread */
void thread_2(void) {
    printf("thread_2: hallo wereld\n");
    while(1);
}

/* copied from libsimple */
void print_bootinfo(seL4_BootInfo *info) {
    /* Parse boot info from kernel. */
    printf("Node ID: %d (of %d)\n",info->nodeID, info->numNodes);
    printf("initThreadCNode size: %d slots\n", (1 << info->initThreadCNodeSizeBits) );

    printf("\n--- Capability Details ---\n");
    printf("Type              Start      End\n");
    printf("Empty             0x%08x 0x%08x\n", info->empty.start, info->empty.end);
    printf("Shared frames     0x%08x 0x%08x\n", info->sharedFrames.start, info->sharedFrames.end);
    printf("User image frames 0x%08x 0x%08x\n", info->userImageFrames.start,
            info->userImageFrames.end);
    printf("User image PTs    0x%08x 0x%08x\n", info->userImagePTs.start, info->userImagePTs.end);
    printf("Untypeds          0x%08x 0x%08x\n", info->untyped.start, info->untyped.end);

    printf("\n--- Untyped Details ---\n");
    printf("Untyped Slot       Paddr      Bits\n");
    for (int i = 0; i < info->untyped.end-info->untyped.start; i++) {
        printf("%3d     0x%08x 0x%08x %d\n", i, info->untyped.start+i, info->untypedPaddrList[i],
                info->untypedSizeBitsList[i]);
    }

    printf("\n--- Device Regions: %d ---\n", info->numDeviceRegions);
    printf("Device Addr     Size Start      End\n");
    for (int i = 0; i < info->numDeviceRegions; i++) {
        printf("%2d 0x%08x %d 0x%08x 0x%08x\n", i,
                                                info->deviceRegions[i].basePaddr,
                                                info->deviceRegions[i].frameSizeBits,
                                                info->deviceRegions[i].frames.start,
                                                info->deviceRegions[i].frames.end);
    }

}

int main(void)
{
    int error;

    /* give us a name: useful for debugging if the thread faults */
    name_thread(seL4_CapInitThreadTCB, "hello-2");

    /* get boot info */
    info = seL4_GetBootInfo();

    /* print out bootinfo */
    print_bootinfo(info);

    /* get our cspace root cnode */
    seL4_CPtr cspace_cap;
    //cspace_cap = simple_get_cnode(&simple);
    cspace_cap = seL4_CapInitThreadCNode;

    /* get our vspace root page diretory */
    seL4_CPtr pd_cap;
    pd_cap = seL4_CapInitThreadPD;

    seL4_CPtr tcb_cap;
    /* TODO 1: Set tcb_cap to a free cap slot index.
     * hint: The bootinfo struct contains a range of free cap slot indices.
     */
    tcb_cap = info->empty.start;

    seL4_CPtr untyped;
    /* TODO 2: Obtain a cap to an untyped which is large enough to contain a tcb.
     *
     * hint 1: determine the size of a tcb object.
     *         (look in libs/libsel4/arch_include/x86/sel4/arch/types.h)
     * hint 2: an array of untyped caps, and a corresponding array of untyped sizes
     *         can be found in the bootinfo struct
     */

    /* look up an untyped to retype into a tcb */
    untyped = get_untyped(info, (1<<seL4_TCBBits));


    /* TODO 3: Retype the untyped into a tcb, storing a cap in tcb_cap
     *
     * hint 1: int seL4_Untyped_Retype(seL4_Untyped service, int type, int size_bits, seL4_CNode root, int node_index, int node_depth, int node_offset, int num_objects)
     * hint 2: use a depth of 32
     * hint 3: use cspace_cap for the root cnode AND the cnode_index
     *         (bonus question: What property of the calling thread's cspace must hold for this to be ok?)
     */

    /* create the tcb */
    assert(!seL4_Untyped_Retype(untyped /* untyped cap */,
                                seL4_TCBObject /* type */, 
                                seL4_TCBBits /* size */, 
                                cspace_cap /* root cnode cap */,
                                cspace_cap /* destination cspace */,
                                32 /* depth */,
                                tcb_cap /* offset */,
                                1 /* num objects */));

    /* initialise the new TCB */
    error = seL4_TCB_Configure(tcb_cap, seL4_CapNull, seL4_MaxPrio,
        cspace_cap, seL4_NilData, pd_cap, seL4_NilData, 0, 0);
    assert(error == 0);

    /* give the new thread a name */
    name_thread(tcb_cap, "hello-2: thread_2");

    uintptr_t thread_2_stack_top = (uintptr_t)thread_2_stack + sizeof(thread_2_stack);
    assert(thread_2_stack_top % (sizeof(seL4_Word) * 2) == 0);
    
    /* TODO 4: Set up regs to contain the desired stack pointer and instruction pointer
     * hint 1: libsel4/arch_include/x86/sel4/arch/types.h:
     *  ...
        typedef struct seL4_UserContext_ {
            seL4_Word eip, esp, eflags, eax, ebx, ecx, edx, esi, edi, ebp;
            seL4_Word tls_base, fs, gs;
        } seL4_UserContext;

     */

    /* set start up registers for the new thread: */
    seL4_UserContext regs = {
        .eip = (seL4_Word)thread_2,
        .esp = (seL4_Word)thread_2_stack_top
    };

    /* TODO 5: Write the registers in regs to the new thread
     *
     * hint 1: int seL4_TCB_WriteRegisters(seL4_TCB service, seL4_Bool resume_target, seL4_Uint8 arch_flags, seL4_Word count, seL4_UserContext *regs);
     * hint 2: the value of arch_flags is ignored on x86 and arm
     */

    /* actually write the TCB registers.  we write 2 registers:
     * instruction pointer is first, stack pointer is second. */
    error = seL4_TCB_WriteRegisters(tcb_cap, 0, 0, 2, &regs);
    assert(error == 0);

    /* start the new thread running */
    error = seL4_TCB_Resume(tcb_cap);
    assert(error == 0);

    /* we are done, say hello */
    printf("main: hello world\n");

    return 0;
}

