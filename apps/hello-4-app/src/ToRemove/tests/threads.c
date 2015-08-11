/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

/* This file contains tests related to TCB syscalls. */

#include <stdio.h>
#include <sel4/sel4.h>

#include "../helpers.h"

int test_tcb_null_cspace_configure(env_t env, void *arg)
{
    helper_thread_t thread;
    int error;

    create_helper_thread(env, &thread);

    /* This should fail because we're passing an invalid CSpace cap. */
    error = seL4_TCB_Configure(thread.thread.tcb.cptr, 0, 100, seL4_CapNull,
                               seL4_CapData_Guard_new(0, 0), env->page_directory,
                               seL4_CapData_Guard_new(0, 0), 0, 0);

    cleanup_helper(env, &thread);

    return error ? SUCCESS : FAILURE;
}
DEFINE_TEST(THREADS0004, "seL4_TCB_Configure with a NULL CSpace should fail", test_tcb_null_cspace_configure)

int test_tcb_null_cspace_setspace(env_t env, void *arg)
{
    helper_thread_t thread;
    int error;

    create_helper_thread(env, &thread);

    /* This should fail because we're passing an invalid CSpace cap. */
    error = seL4_TCB_SetSpace(thread.thread.tcb.cptr, 0, seL4_CapNull,
                              seL4_CapData_Guard_new(0, 0), env->page_directory,
                              seL4_CapData_Guard_new(0, 0));

    cleanup_helper(env, &thread);

    return error ? SUCCESS : FAILURE;
}
DEFINE_TEST(THREADS0005, "seL4_TCB_SetSpace with a NULL CSpace should fail", test_tcb_null_cspace_setspace)
