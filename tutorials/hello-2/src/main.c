/*
 * Copyright 2017, Data61
 * Commonwealth Scientific and Industrial Research Organisation (CSIRO)
 * ABN 41 687 119 230.
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(DATA61_BSD)
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

#include <utils/arith.h>
#include <utils/zf_log.h>
#include <sel4utils/sel4_zf_logif.h>

#include <sel4platsupport/bootinfo.h>

/* global environment variables */

/* seL4_BootInfo defined in bootinfo.h
 * Links to source: https://docs.sel4.systems/Tutorials/seL4_Tutorial_2#globals-links: */
seL4_BootInfo *info;

/* simple_t defined in simple.h
 * Links to source: https://docs.sel4.systems/Tutorials/seL4_Tutorial_2#globals-links: */
simple_t simple;

/* vka_t defined in vka.h
 * Links to source: https://docs.sel4.systems/Tutorials/seL4_Tutorial_2#globals-links: */
vka_t vka;

/* allocman_t defined in allocman.h
 * Links to source: https://docs.sel4.systems/Tutorials/seL4_Tutorial_2#globals-links: */
allocman_t *allocman;

/* static memory for the allocator to bootstrap with */
#define ALLOCATOR_STATIC_POOL_SIZE (BIT(seL4_PageBits) * 10)
UNUSED static char allocator_mem_pool[ALLOCATOR_STATIC_POOL_SIZE];

/* stack for the new thread */
#define THREAD_2_STACK_SIZE 512
static uint64_t thread_2_stack[THREAD_2_STACK_SIZE];

/* name_thread(): convenience function in util.c:
 * Links to source: https://docs.sel4.systems/Tutorials/seL4_Tutorial_2#globals-links:
 */
extern void name_thread(seL4_CPtr tcb, char *name);

/* function to run in the new thread */
void thread_2(void) {
    /*? include_task_type_append(["task-15"]) ?*/

}

int main(void) {
    UNUSED int error = 0;

    /*? include_task("task-1") ?*/

    /* Set up logging and give us a name: useful for debugging if the thread faults */
    /* seL4_CapInitThreadTCB is a cap pointer to the root task's initial TCB.
     * It is part of the root task's boot environment and defined in bootinfo.h from libsel4:
     * https://docs.sel4.systems/Tutorials/seL4_Tutorial_2#globals-links:
     */
    zf_log_set_tag_prefix("hello-2:");
    name_thread(seL4_CapInitThreadTCB, "hello-2");
    /*- set tasks = [] -*/
    /*-- for i in range(2,15) -*/
    /*-- if tasks.append("task-%d" %i) -*/ /*- endif -*/
    /*-- endfor -*/
    /*? include_task_type_append(tasks) ?*/

    printf("main: hello world\n");

    return 0;
}
