/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

#include <sel4/sel4.h>

#include "../helpers.h"

static int
remote_function(void)
{
    return 42;
}

static int
test_interas_diffcspace(env_t env, void *args)
{
    helper_thread_t t;

    create_helper_process(env, &t);

    start_helper(env, &t, (helper_fn_t) remote_function, 0, 0, 0, 0);
    seL4_Word ret = wait_for_helper(&t);
    test_assert(ret == 42);
    cleanup_helper(env, &t);

    return SUCCESS;
}
DEFINE_TEST(VSPACE0000, "Test threads in different cspace/vspace", test_interas_diffcspace)
