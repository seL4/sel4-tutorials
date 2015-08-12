/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

/* Include Kconfig variables. */
#include <autoconf.h>

#include <assert.h>
#include <stdio.h>

/* Our headers are not C++ friendly */
extern "C" {

#include <sel4/sel4.h>

#include "../helpers.h"

}

#define POLL_DELAY_NS 2000

typedef int (*test_func_t)(seL4_Word /* id */, env_t env /* env */);

static int
own_domain_success(struct env *env)
{
    int error;

    error = seL4_DomainSet_Set(env->domain, CONFIG_NUM_DOMAINS - 1,
                               env->tcb);
    return (error == seL4_NoError) ? SUCCESS : FAILURE;
}

static int
own_domain_baddom(struct env *env)
{
    int error;

    error = seL4_DomainSet_Set(env->domain, CONFIG_NUM_DOMAINS + 10,
                               env->tcb);
    return (error == seL4_InvalidArgument) ? SUCCESS : FAILURE;
}

static int
own_domain_badcap(struct env *env)
{
    int error;

    error = seL4_DomainSet_Set(env->tcb, 0,
                               env->tcb);
    return (error == seL4_IllegalOperation) ? SUCCESS : FAILURE;
}

#ifdef CONFIG_HAVE_TIMER
void
wait(int nsecs, env_t env)
{

    UNUSED int error = timer_oneshot_relative(env->timer->timer, nsecs);
    assert(error == 0);

    wait_for_timer_interrupt(env);
}

int fdom1(seL4_Word id, env_t env)
{
    int countdown = 50;

    while (countdown > 0) {
        wait(POLL_DELAY_NS, env);
        --countdown;
        printf("%2d, ", id);
    }

    return SUCCESS;
}

/* This is a very simple (and rather stupid) C++ usage. Proves that a template
 * can be defined but is not good C++ */
template<bool shift, typename F>
static int
test_domains(struct env *env, F func)
{

    UNUSED int error;
    helper_thread_t thread[CONFIG_NUM_DOMAINS];

    timer_start(env->timer->timer);
    sel4_timer_handle_single_irq(env->timer);

    for (int i = 0; i < CONFIG_NUM_DOMAINS; ++i) {
        create_helper_thread(env, &thread[i]);
        set_helper_priority(&thread[i], 64);
        error = seL4_DomainSet_Set(env->domain, (seL4_Word)i, thread[i].thread.tcb.cptr);
        assert(error == seL4_NoError);
    }

    for (int i = 0; i < CONFIG_NUM_DOMAINS; ++i) {
        start_helper(env, &thread[i], (helper_fn_t) func, i, (seL4_Word) env, 0, 0);
    }

    if (CONFIG_NUM_DOMAINS > 1 && shift) {
        assert(0);
        wait(POLL_DELAY_NS * 2, env);
        error = seL4_DomainSet_Set(env->domain, CONFIG_NUM_DOMAINS - 1, thread[0].thread.tcb.cptr);
        assert(error == seL4_NoError);
    }

    for (int i = 0; i < CONFIG_NUM_DOMAINS; ++i) {
        wait_for_helper(&thread[i]);
        cleanup_helper(env, &thread[i]);
    }

    timer_stop(env->timer->timer);
    sel4_timer_handle_single_irq(env->timer);
    return SUCCESS;
}


/* The output from this test should show alternating "domain blocks", with,
 * within each, a single thread printing. For example:
 *
 * 00, 00, ..., 00, 01, 01, ..., 01, 00, 00, ..., 00, 01, 01, ..., 01, etc.
 * +-------------+  +-------------+  +-------------+  +-------------+
 *  block 0           block 1          block 0          block 1
 */
static int
test_run_domains(struct env* env, void *args)
{
    return test_domains<false>(env, fdom1);
}
DEFINE_TEST(DOMAINS0004, "Run threads in domains()", test_run_domains)

#if CONFIG_NUM_DOMAINS > 1
/* The output of this test differs from that of DOMAINS0004 in that the thread
 * in domain 0 is moved into domain 1 after a short delay. This should be
 * visible in the output, where the "domain block" for domain 1 should contain
 * the alternating output of threads 0 and 1. For example:
 *
 * 00, 00, ..., 00, 01, 00, 01, 00, ..., 01, 01, 01, 01
 * +-------------+  +------------------+ +------------+
 *  initial block    alterations           final block
 *  (due to delay)   (after shift)         (01 catches up)
 */
static int
test_run_domains_shift(struct env* env, void *args)
{
    return test_domains<true>(env, fdom1);
}
DEFINE_TEST(DOMAINS0005, "Move thread between domains()", test_run_domains_shift)
#endif
#endif /* CONFIG_HAVE_TIMER */

static int
test_own_domain1(struct env* env, void *args)
{
    return own_domain_success(env);
}
DEFINE_TEST(DOMAINS0001, "Change domain successfully()", test_own_domain1)

static int
test_own_domain2(struct env* env, void *args)
{
    return own_domain_baddom(env);
}
DEFINE_TEST(DOMAINS0002, "Try non-existant domain()", test_own_domain2)

static int
test_own_domain3(struct env* env, void *args)
{
    return own_domain_badcap(env);
}
DEFINE_TEST(DOMAINS0003, "Invoke non-domain cap()", test_own_domain3)
