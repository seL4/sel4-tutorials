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
 * Part X: create a new thread
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


seL4_BootInfo *info;
simple_t simple;
vka_t vka;
allocman_t *allocman;

/* static memory for the allocator to bootstrap with */
#define ALLOCATOR_STATIC_POOL_SIZE ((1 << seL4_PageBits) * 10)
UNUSED static char allocator_mem_pool[ALLOCATOR_STATIC_POOL_SIZE];

#define THREAD_2_STACK_SIZE 4096
static int thread_2_stack[THREAD_2_STACK_SIZE];

void abort(void) {
    while (1);
}

void __arch_putchar(int c) {
#ifdef CONFIG_DEBUG_BUILD
    seL4_DebugPutChar(c);
#endif
}

void thread_2(void *a) {
    printf("hallo wereld\n");
    while(1);
}

int main(void)
{
    UNUSED int error;

#ifdef SEL4_DEBUG_KERNEL
    seL4_DebugNameThread(seL4_CapInitThreadTCB, "hello-libs-2");
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

    /* initialise the TCB */
    error = seL4_TCB_Configure(tcb_object.cptr /* service */, 
                               seL4_CapNull /* fault ep */, 
                               seL4_MaxPrio /* priority */,
                               cspace_cap /* cspace root */, 
                               seL4_NilData /* cspace root data */, 
                               pd_cap /* vspace root */,
                               seL4_NilData /* vspace root data */, 
                               0 /* buffer */, 
                               0 /* buffer frame */);
    assert(error == 0);

#ifdef SEL4_DEBUG_KERNEL
    seL4_DebugNameThread(tcb_object.cptr, "hello-libs-2: thread_2");
#endif

    seL4_UserContext regs;
    error = seL4_TCB_ReadRegisters(tcb_object.cptr, 0, 0, 2, &regs);
    assert(error == 0);
    sel4utils_set_instruction_pointer(&regs, (seL4_Word)thread_2);
    sel4utils_set_stack_pointer(&regs, (seL4_Word)thread_2_stack);

/*    seL4_TCB_WriteRegisters(seL4_TCB service, seL4_Bool resume_target, seL4_Uint8 arch_flags, seL4_Word count, seL4_UserContext *regs)
      count: number of registers to transfer. IP is first, SP is second.
*/
    error = seL4_TCB_WriteRegisters(tcb_object.cptr, 0, 0, 2, &regs);
    assert(error == 0);

    /* start it running */
    error = seL4_TCB_Resume(tcb_object.cptr);
    assert(error == 0);

    printf("hello world\n");

    return 0;
}

