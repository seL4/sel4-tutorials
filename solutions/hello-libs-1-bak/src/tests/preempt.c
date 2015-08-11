/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

#include <assert.h>
#include <stdio.h>
#include <sel4/sel4.h>
#include <vka/object.h>

#include "../helpers.h"

#if 0
#define D(args...) printf(args)
#else
#define D(args...)
#endif

#if CONFIG_HAVE_TIMER

static volatile int revoking = 0;
static volatile int preempt_count = 0;

static int
revoke_func(seL4_CNode service, seL4_Word index, seL4_Word depth)
{
    revoking = 1;
    seL4_CNode_Revoke(service, index, depth);
    revoking = 2;
    return 0;
}

static int
preempt_count_func(env_t env)
{
    while (revoking < 2) {
        wait_for_timer_interrupt(env);
        if (revoking == 1) {
            preempt_count++;
        }
    }
    return 0;
}

static int
test_preempt_revoke_actual(env_t env, int num_cnode_bits)
{
    helper_thread_t revoke_thread, preempt_thread;
    int error;

    /* Create an endpoint cap that will be derived many times. */
    seL4_CPtr ep = vka_alloc_endpoint_leaky(&env->vka);

    create_helper_thread(env, &revoke_thread);
    create_helper_thread(env, &preempt_thread);

    set_helper_priority(&preempt_thread, 101);
    set_helper_priority(&revoke_thread, 100);

    /* Now create as many cnodes as possible. We will copy the cap into all
     * those cnodes. */
    int num_caps = 0;

#define CNODE_SIZE_BITS 12

    D("    Creating %d caps .", (1 << (num_cnode_bits + CNODE_SIZE_BITS)));
    for (int i = 0; i < (1 << num_cnode_bits); i++) {
        seL4_CPtr ctable = vka_alloc_cnode_object_leaky(&env->vka, CNODE_SIZE_BITS);

        for (int i = 0; i < (1 << CNODE_SIZE_BITS); i++) {
            error = seL4_CNode_Copy(
                        ctable, i, CNODE_SIZE_BITS,
                        env->cspace_root, ep, seL4_WordBits,
                        seL4_AllRights);

            test_assert_fatal(!error);
            num_caps++;
        }
        D(".");
    }
    test_check(num_caps > 0);
    test_check((num_caps == (1 << (num_cnode_bits + CNODE_SIZE_BITS))));

    timer_periodic(env->timer->timer, NS_IN_MS);
    timer_start(env->timer->timer);
    sel4_timer_handle_single_irq(env->timer);

    /* Last thread to start runs first. */
    revoking = 0;
    preempt_count = 0;
    start_helper(env, &preempt_thread, (helper_fn_t) preempt_count_func, (seL4_Word) env, 0, 0, 0);
    start_helper(env, &revoke_thread, (helper_fn_t) revoke_func, env->cspace_root,
                 ep, seL4_WordBits, 0);

    wait_for_helper(&revoke_thread);

    cleanup_helper(env, &preempt_thread);
    cleanup_helper(env, &revoke_thread);

    D("    %d preemptions\n", preempt_count);

    timer_stop(env->timer->timer);
    sel4_timer_handle_single_irq(env->timer);

    return preempt_count;
}

static int
test_preempt_revoke(env_t env, void *args)
{
    for (int num_cnode_bits = 1; num_cnode_bits < 8; num_cnode_bits++) {
        if (test_preempt_revoke_actual(env, num_cnode_bits) > 1) {
            return SUCCESS;
        }
    }

    D("Couldn't trigger preemption point with millions of caps!\n");
    test_assert(0);
}
DEFINE_TEST(PREEMPT_REVOKE, "Test preemption path in revoke", test_preempt_revoke)
#endif
