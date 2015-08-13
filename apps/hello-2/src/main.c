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

#include <sel4/sel4.h>

#include <simple/simple.h>
#include <simple-default/simple-default.h>

#include <vka/object.h>

#include <allocman/allocman.h>
#include <allocman/bootstrap.h>
#include <allocman/vka.h>


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
    /* TODO: print something: hint printf() */
}

int main(void)
{
    /* give us a name: useful for debugging if the thread faults */
    name_thread(seL4_CapInitThreadTCB, "hello-2");

    /* TODO: get boot info: hint seL4_GetBootInfo() */

    /* TODO: init simple: hint simple_default_init_bootinfo() */

    /* TODO: print out bootinfo and other info about simple: hint simple_print() */

    /* TODO: create an allocator: hint bootstrap_use_current_simple() */

    /* TODO: create a vka (interface for interacting with the underlying allocator)
     * hint: allocman_make_vka() */

    /* TODO: get our cspace root cnode: hint simple_get_cnode() */

    /* TODO: get our vspace root page diretory: hint simple_get_pd() */

    /* TODO: create a new TCB: hint vka_alloc_tcb() */

    /* TODO: initialise the new TCB:
     * hint 1: seL4_TCB_Configure()
     * hint 2: use seL4_CapNull for the fault endpoint
     * hint 3: use seL4_NilData for cspace and vspace data
     * hint 4: we don't need an IPC buffer frame or address yet */

    /* TODO: give the new thread a name */

    /*
     * set start up registers for the new thread:
     */

    /* TODO: set instruction pointer where the thread shoud start running
     * hint: sel4utils_set_instruction_pointer() */

    /* TODO: set stack pointer for the new thread.
     * hint 1: sel4utils_set_stack_pointer
     * hint 2: remember the stack grows down */

    /* TODO: actually write the TCB registers.  we write 2 registers:
     * instruction pointer is first, stack pointer is second.
     * hint: seL4_TCB_WriteRegisters() */

    /* TODO: start the new thread running: hint seL4_TCB_Resume() */

    /* we are done, say hello */
    printf("main: hello world\n");

    return 0;
}

