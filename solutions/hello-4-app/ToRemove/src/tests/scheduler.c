/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

#include <stdio.h>
#include <sel4/sel4.h>
#include <stdlib.h>
#include <string.h>
#include <vka/object.h>

#include "../helpers.h"

#define PRIORITY_FUDGE 1

#define MIN_PRIO seL4_MinPrio
#define MAX_PRIO (OUR_PRIO - 1)
#define NUM_PRIOS (MAX_PRIO - MIN_PRIO + 1)

#if 0
#define D(args...) printf(args)
#else
#define D(args...)
#endif

/*
 * Performing printf on non-debug x86 will perform IPCs. So, eliminate printf
 * there, but keep it everywhere else where it's useful.
 */
#if defined(ARCH_IA32) && !defined(SEL4_DEBUG_KERNEL)
#define SAFE_PRINTF(x...) do { } while(0)
#else
#define SAFE_PRINTF(x...) D(x)
#endif

#define CHECK_STEP(var, x) do { \
        test_check((var) == x); \
        SAFE_PRINTF(#x "..."); \
        var = (x) + 1; \
    } while (0)

#define CHECK_TESTCASE(result, condition) \
    do { \
        int _condition = !!(condition); \
        test_check(_condition); \
        (result) |= !_condition; \
    } while (0)

/* We need a timer to run this test. However this test gets run on simulators,
 * of which the beagle does not have a properly emulated timer and results
 * in this test failing. So we don't include this test if running the beagle */
#if CONFIG_HAVE_TIMER && !defined(CONFIG_PLAT_OMAP3)
static int
counter_func(volatile seL4_Word *counter)
{
    while (1) {
        (*counter)++;
    }
    return 0;
}

/*
 * Test that thread suspending works when idling the system.
 * Note: This test non-deterministically fails. If you find only this test
 * try re-running the test suite.
 */
static int
test_thread_suspend(env_t env, void *args)
{
    helper_thread_t t1;
    volatile seL4_Word counter;
    D("test_thread_suspend\n");
    create_helper_thread(env, &t1);

    set_helper_priority(&t1, 100);
    start_helper(env, &t1, (helper_fn_t) counter_func, (seL4_Word) &counter, 0, 0, 0);

    timer_periodic(env->timer->timer, 10 * NS_IN_MS);
    timer_start(env->timer->timer);
    sel4_timer_handle_single_irq(env->timer);

    seL4_Word old_counter;

    /* Let the counter thread run. We might have a pending interrupt, so
     * wait twice. */
    wait_for_timer_interrupt(env);
    wait_for_timer_interrupt(env);

    old_counter = counter;

    /* Let it run again. */
    wait_for_timer_interrupt(env);

    /* Now, counter should have moved. */
    test_check(counter != old_counter);
    old_counter = counter;

    /* Suspend the thread, and wait again. */
    seL4_TCB_Suspend(t1.thread.tcb.cptr);
    wait_for_timer_interrupt(env);

    /* Counter should not have moved. */
    test_check(counter == old_counter);
    old_counter = counter;

    /* Check once more for good measure. */
    wait_for_timer_interrupt(env);

    /* Counter should not have moved. */
    test_check(counter == old_counter);
    old_counter = counter;

    /* Resume the thread and check it does move. */
    seL4_TCB_Resume(t1.thread.tcb.cptr);
    wait_for_timer_interrupt(env);
    test_check(counter != old_counter);

    /* Done. */
    timer_stop(env->timer->timer);
    sel4_timer_handle_single_irq(env->timer);
    cleanup_helper(env, &t1);

    return SUCCESS;
}
DEFINE_TEST(SCHED0000, "Test suspending and resuming a thread (flaky)", test_thread_suspend)
#endif /* CONFIG_HAVE_TIMER */

#ifdef CONFIG_KERNEL_STABLE

/*
 * Test setting priorities.
 */
static int
test_set_priorities(struct env* env, void *args)
{
    /* Ensure we can set our priority equal to ourselves. */
    int error = seL4_TCB_SetPriority(env->tcb, OUR_PRIO);
    test_assert(!error);

    /* Ensure we can set a different thread's priority equal to ourselves. */
    seL4_CPtr tcb = vka_alloc_tcb_leaky(&env->vka);
    error = seL4_TCB_SetPriority(tcb, OUR_PRIO);
    test_assert(!error);

    return SUCCESS;
}
DEFINE_TEST(SCHED0001, "Test setting priorities", test_set_priorities)

#endif /* CONFIG_KERNEL_STABLE */

/*
 * Test TCB Resume on self.
 */
static int
test_resume_self(struct env* env, void *args)
{
    D("Starting test_resume_self\n");
    /* Ensure nothing bad happens if we resume ourselves. */
    int error = seL4_TCB_Resume(env->tcb);
    test_assert(!error);
    D("Ending test_resume_self\n");
    return SUCCESS;
}
DEFINE_TEST(SCHED0002, "Test resuming ourselves", test_resume_self)

/*
 * Test TCB Suspend/Resume.
 */
static volatile int suspend_test_step;
static int
suspend_test_helper_2a(seL4_CPtr *t1, seL4_CPtr *t2a, seL4_CPtr *t2b)
{
    /* Helper function that runs at a higher priority. */

    /* Start here. */
    CHECK_STEP(suspend_test_step, 0);

    /* Wait for a timer tick to make 2b run. */
    while (suspend_test_step == 1)
        /* spin */{
        ;
    }

    /* Suspend helper 2b. */
    int error = seL4_TCB_Suspend(*t2b);
    test_check(!error);

    CHECK_STEP(suspend_test_step, 2);

    /* Now suspend ourselves, passing control to the low priority process to
     * resume 2b. */
    error = seL4_TCB_Suspend(*t2a);
    test_check(!error);

    return SUCCESS;
}

static int
suspend_test_helper_2b(seL4_CPtr *t1, seL4_CPtr *t2a, seL4_CPtr *t2b)
{
    /* Wait for 2a to get suspend_test_step set to 1. */
    test_check(suspend_test_step == 0 || suspend_test_step == 1);
    while (suspend_test_step == 0) {
        seL4_Yield();
    }

    /* Timer tick should bring us back here. */
    CHECK_STEP(suspend_test_step, 1);

    /* Now spin and wait for us to be suspended. */
    while (suspend_test_step == 2) {
        seL4_Yield();
    }

    /* When we wake up suspend_test_step should be 4. */
    CHECK_STEP(suspend_test_step, 4);

    return SUCCESS;
}

static int
suspend_test_helper_1(seL4_CPtr *t1, seL4_CPtr *t2a, seL4_CPtr *t2b)
{
    CHECK_STEP(suspend_test_step, 3);

    /* Our sole job is to wake up 2b. */
    int error = seL4_TCB_Resume(*t2b);
    test_check(!error);

    /* We should have been preempted immediately, so by the time we run again,
     * the suspend_test_step should be 5. */

#if 1 // WE HAVE A BROKEN SCHEDULER IN SEL4
    /* FIXME: The seL4 scheduler is broken, and seL4_TCB_Resume will not
     * preempt. The only way to get preempted is to force it ourselves (or wait
     * for a timer tick). */
    seL4_Yield();
#endif

    CHECK_STEP(suspend_test_step, 5);

    return SUCCESS;
}

#ifndef CONFIG_FT

static int
test_suspend(struct env* env, void *args)
{
    helper_thread_t thread1;
    helper_thread_t thread2a;
    helper_thread_t thread2b;

    D("Starting test_suspend\n");

    create_helper_thread(env, &thread1);
    D("Show me\n");
    create_helper_thread(env, &thread2a);

    create_helper_thread(env, &thread2b);

    /* First set all the helper threads to have unique priorities
     * and then start them in order of priority. This is so when
     * the 'start_helper' function does an IPC to the helper
     * thread, it doesn't allow one of the already started helper
     * threads to run at all */
    set_helper_priority(&thread1, 0);
    set_helper_priority(&thread2a, 1);
    set_helper_priority(&thread2b, 2);

    start_helper(env, &thread1, (helper_fn_t) suspend_test_helper_1,
                 (seL4_Word) &thread1.thread.tcb.cptr,
                 (seL4_Word) &thread2a.thread.tcb.cptr,
                 (seL4_Word) &thread2b.thread.tcb.cptr, 0);

    start_helper(env, &thread2a, (helper_fn_t) suspend_test_helper_2a,
                 (seL4_Word) &thread1.thread.tcb.cptr,
                 (seL4_Word) &thread2a.thread.tcb.cptr,
                 (seL4_Word) &thread2b.thread.tcb.cptr, 0);

    start_helper(env, &thread2b, (helper_fn_t) suspend_test_helper_2b,
                 (seL4_Word) &thread1.thread.tcb.cptr,
                 (seL4_Word) &thread2a.thread.tcb.cptr,
                 (seL4_Word) &thread2b.thread.tcb.cptr, 0);

    /* Now set their priorities to what we want */
    set_helper_priority(&thread1, 100);
    set_helper_priority(&thread2a, 101);
    set_helper_priority(&thread2b, 101);

    suspend_test_step = 0;
    D("      ");

    wait_for_helper(&thread1);
    wait_for_helper(&thread2b);

    CHECK_STEP(suspend_test_step, 6);
    D("\n");

    cleanup_helper(env, &thread1);
    cleanup_helper(env, &thread2a);
    cleanup_helper(env, &thread2b);

    return SUCCESS;
}
DEFINE_TEST(SCHED0003, "Test TCB suspend/resume", test_suspend)

#endif /* !CONFIG_FT */

/*
 * Test threads at all possible priorities, and that they get scheduled in the
 * correct order.
 */
static int
prio_test_func(seL4_Word my_prio, seL4_Word* last_prio, seL4_CPtr ep)
{
    test_check(*last_prio - 1 == my_prio);

    *last_prio = my_prio;

    /* Unsuspend the top thread if we are the last one. */
    if (my_prio == MIN_PRIO) {
        seL4_MessageInfo_t tag = seL4_MessageInfo_new(0, 0, 0, 0);
        seL4_Send(ep, tag);
    }
    return 0;
}


static int
test_all_priorities(struct env* env, void *args)
{
    int i;

    helper_thread_t **threads = (helper_thread_t **) malloc(sizeof(helper_thread_t*) * NUM_PRIOS);
    assert(threads != NULL);

    for (i = 0; i < NUM_PRIOS; i++) {
        threads[i] = (helper_thread_t*) malloc(sizeof(helper_thread_t));
        assert(threads[i]);
    }

    vka_t *vka = &env->vka;
    D("Testing all thread priorities");
    volatile seL4_Word last_prio = MAX_PRIO + 1;

    seL4_CPtr ep = vka_alloc_endpoint_leaky(vka);

    for (int prio = MIN_PRIO; prio <= MAX_PRIO; prio++) {
        int idx = prio - MIN_PRIO;
        test_check(idx >= 0 && idx < NUM_PRIOS);
        create_helper_thread(env, threads[idx]);
        set_helper_priority(threads[idx], prio);

        start_helper(env, threads[idx], (helper_fn_t) prio_test_func,
                     prio, (seL4_Word) &last_prio, ep, 0);
    }

    /* Now block. */
    seL4_Word sender_badge = 0;
    seL4_Wait(ep, &sender_badge);

    /* When we get woken up, last_prio should be MIN_PRIO. */
    test_check(last_prio == MIN_PRIO);

    for (int prio = MIN_PRIO; prio <= MAX_PRIO; prio++) {
        int idx = prio - MIN_PRIO;
        cleanup_helper(env, threads[idx]);
        free(threads[idx]);
    }
    free(threads);

    return SUCCESS;
}
DEFINE_TEST(SCHED0004, "Test threads at all priorities", test_all_priorities)

/*
 * Test setting the priority of a runnable thread.
 */
static volatile int set_priority_step;
static int
set_priority_helper_1(seL4_CPtr *t1, seL4_CPtr *t2)
{
    test_check(set_priority_step == 0);
    D("0...");
    set_priority_step = 1;

    /*
     * Down our priority. This should force a reschedule and make thread 2 run.
     */
    int error = seL4_TCB_SetPriority(*t1, 100);
    test_check(!error);

    test_check(set_priority_step == 2);
    D("2...");
    set_priority_step = 3;

    return 0;
}

static int
set_priority_helper_2(seL4_CPtr *t1, seL4_CPtr *t2)
{
    test_check(set_priority_step == 1);
    D("1...");

    /* Raise thread 1 to equal to ours, which should fail. */
    int error = seL4_TCB_SetPriority(*t1, 110 + PRIORITY_FUDGE);
    test_check(error == seL4_IllegalOperation);

    /* Raise thread 1 to just below us. */
    error = seL4_TCB_SetPriority(*t1, 109);
    test_check(!error);

    /* Drop ours to below thread 1. Thread 1 should run. */
    set_priority_step = 2;
    error = seL4_TCB_SetPriority(*t2, 108);
    test_check(!error);

    /* Once thread 1 exits, we should run. */
    test_check(set_priority_step == 3);
    D("3...");
    set_priority_step = 4;

    return 0;
}

static int
test_set_priority(struct env* env, void *args)
{
    helper_thread_t thread1;
    helper_thread_t thread2;
    D("test_set_priority starting\n");
    create_helper_thread(env, &thread1);
    set_helper_priority(&thread1, 254);

    create_helper_thread(env, &thread2);
    set_helper_priority(&thread2, 110);

    set_priority_step = 0;
    D("      ");

    start_helper(env, &thread1, (helper_fn_t) set_priority_helper_1,
                 (seL4_Word) &thread1.thread.tcb.cptr,
                 (seL4_Word) &thread2.thread.tcb.cptr, 0, 0);

    start_helper(env, &thread2, (helper_fn_t) set_priority_helper_2,
                 (seL4_Word) &thread1.thread.tcb.cptr,
                 (seL4_Word) &thread2.thread.tcb.cptr, 0, 0);

    wait_for_helper(&thread1);
    wait_for_helper(&thread2);

    test_check(set_priority_step == 4);

    D("\n");
    cleanup_helper(env, &thread1);
    cleanup_helper(env, &thread2);
    return SUCCESS;
}
DEFINE_TEST(SCHED0005, "Test set priority", test_set_priority)

/*
 * Perform IPC Send operations across priorities and ensure that strict
 * priority-based scheduling is still observed.
 */
static volatile int ipc_test_step;
typedef struct ipc_test_data {
    volatile seL4_CPtr ep0, ep1, ep2, ep3;
    volatile uint32_t bounces;
    volatile uint32_t spins;
    seL4_CPtr tcb0, tcb1, tcb2, tcb3;
} ipc_test_data_t;

static int
ipc_test_helper_0(ipc_test_data_t *data)
{
    /* We are a "bouncer" thread. Each time a high priority process actually
     * wants to wait for a low priority process to execute and block, it does a
     * call to us. We are the lowest priority process and therefore will run
     * only after all other higher priority threads are done.
     */
    while (1) {
        seL4_MessageInfo_t tag;
        seL4_Word sender_badge = 0;
        tag = seL4_Wait(data->ep0, &sender_badge);
        data->bounces++;
        seL4_Reply(tag);
    }

    return 0;
}

static int
ipc_test_helper_1(ipc_test_data_t *data)
{
    seL4_Word sender_badge = 0;
    seL4_MessageInfo_t tag;
    int result = 0;

    /* TEST PART 1 */
    /* Receive a pending send. */
    CHECK_STEP(ipc_test_step, 1);
    tag = seL4_Wait(data->ep1, &sender_badge);

    /* As soon as the wait is performed, we should be preempted. */

    /* Thread 3 will give us a chance to check our message. */
    CHECK_STEP(ipc_test_step, 3);
    CHECK_TESTCASE(result, seL4_MessageInfo_get_length(tag) == 20);
    for (int i = 0; i < seL4_MessageInfo_get_length(tag); i++) {
        CHECK_TESTCASE(result, seL4_GetMR(i) == i);
    }

    /* Now we bounce to allow thread 3 control again. */
    seL4_MessageInfo_ptr_set_length(&tag, 0);
    seL4_Call(data->ep0, tag);

    /* TEST PART 2 */
    /* Receive a send that is not yet pending. */
    CHECK_STEP(ipc_test_step, 5);
    tag = seL4_Wait(data->ep1, &sender_badge);

    CHECK_STEP(ipc_test_step, 8);
    CHECK_TESTCASE(result, seL4_MessageInfo_get_length(tag) == 19);
    for (int i = 0; i < seL4_MessageInfo_get_length(tag); i++) {
        CHECK_TESTCASE(result, seL4_GetMR(i) == i);
    }

    return result;
}

static int
ipc_test_helper_2(ipc_test_data_t *data)
{
    /* We are a spinner thread. Our job is to do spin, and occasionally bounce
     * to thread 0 to let other code execute. */
    while (1) {
        /* Ensure nothing happens whilst we are busy. */
        int last_step = ipc_test_step;
        for (int i = 0; i < 100000; i++) {
            asm volatile ("");
        }
        test_check(last_step == ipc_test_step);

        data->spins++;

        /* Bounce. */
        seL4_MessageInfo_t tag = seL4_MessageInfo_new(0, 0, 0, 0);
        seL4_Call(data->ep0, tag);
    }
    return 0;
}

static int
ipc_test_helper_3(ipc_test_data_t *data)
{
    seL4_MessageInfo_t tag;
    int last_spins, last_bounces, result = 0;

    /* This test starts here. */

    /* TEST PART 1 */
    /* Perform a send to a thread 1. It is not yet waiting. */
    CHECK_STEP(ipc_test_step, 0);
    seL4_MessageInfo_ptr_new(&tag, 0, 0, 0, 20);
    for (int i = 0; i < seL4_MessageInfo_get_length(tag); i++) {
        seL4_SetMR(i, i);
    }
    last_spins = data->spins;
    last_bounces = data->bounces;
    seL4_Send(data->ep1, tag);
    /* We block, causing thread 2 to spin for a while, before it calls the
     * bouncer thread 0, which finally lets thread 1 run and reply to us. */
    CHECK_STEP(ipc_test_step, 2);
    CHECK_TESTCASE(result, data->spins - last_spins == 1);
    CHECK_TESTCASE(result, data->bounces - last_bounces == 0);

    /* Now bounce ourselves, to ensure that thread 1 can check its stuff. */
    seL4_MessageInfo_ptr_set_length(&tag, 0);
    seL4_Call(data->ep0, tag);

    /* Two bounces - us and thread 1. */
    CHECK_TESTCASE(result, data->spins - last_spins == 2);
    CHECK_TESTCASE(result, data->bounces - last_bounces == 2);
    CHECK_STEP(ipc_test_step, 4);

    /* TEST PART 2 */
    /* Perform a send to a thread 1, which is already waiting. */
    /* Bounce first to let thread prepare. */
    last_spins = data->spins;
    last_bounces = data->bounces;

    seL4_MessageInfo_ptr_set_length(&tag, 0);
    seL4_Call(data->ep0, tag);
    CHECK_STEP(ipc_test_step, 6);

    /* Do the send. */
    seL4_MessageInfo_ptr_set_length(&tag, 19);
    for (int i = 0; i < seL4_MessageInfo_get_length(tag); i++) {
        seL4_SetMR(i, i);
    }
    seL4_Send(data->ep1, tag);

    CHECK_STEP(ipc_test_step, 7);

    /* Bounce to let thread 1 check again. */
    seL4_MessageInfo_ptr_set_length(&tag, 0);
    seL4_Call(data->ep0, tag);

    CHECK_STEP(ipc_test_step, 9);

    /* Five bounces in total. */
    CHECK_TESTCASE(result, data->spins - last_spins == 2);
    CHECK_TESTCASE(result, data->bounces - last_bounces == 5);

    return result;
}

static int
test_ipc_prios(struct env* env, void *args)
{
    vka_t *vka = &env->vka;
    helper_thread_t thread0;
    helper_thread_t thread1;
    helper_thread_t thread2;
    helper_thread_t thread3;
    int result = 0;

    ipc_test_data_t data;
    memset(&data, 0, sizeof(data));

    data.ep0 = vka_alloc_endpoint_leaky(vka);
    data.ep1 = vka_alloc_endpoint_leaky(vka);
    data.ep2 = vka_alloc_endpoint_leaky(vka);
    data.ep3 = vka_alloc_endpoint_leaky(vka);

    create_helper_thread(env, &thread0);
    set_helper_priority(&thread0, 0);

    create_helper_thread(env, &thread1);
    set_helper_priority(&thread1, 1);

    create_helper_thread(env, &thread2);
    set_helper_priority(&thread2, 2);

    create_helper_thread(env, &thread3);
    set_helper_priority(&thread3, 3);

    data.tcb0 = thread0.thread.tcb.cptr;
    data.tcb1 = thread1.thread.tcb.cptr;
    data.tcb2 = thread2.thread.tcb.cptr;
    data.tcb3 = thread3.thread.tcb.cptr;

    D("      ");
    ipc_test_step = 0;

    start_helper(env, &thread0, (helper_fn_t) ipc_test_helper_0, (seL4_Word) &data, 0, 0, 0);
    start_helper(env, &thread1, (helper_fn_t) ipc_test_helper_1, (seL4_Word) &data, 0, 0, 0);
    start_helper(env, &thread2, (helper_fn_t) ipc_test_helper_2, (seL4_Word) &data, 0, 0, 0);
    start_helper(env, &thread3, (helper_fn_t) ipc_test_helper_3, (seL4_Word) &data, 0, 0, 0);

    result |= (int)wait_for_helper(&thread1);
    result |= (int)wait_for_helper(&thread3);

    CHECK_STEP(ipc_test_step, 10);
    D("\n");

    cleanup_helper(env, &thread0);
    cleanup_helper(env, &thread1);
    cleanup_helper(env, &thread2);
    cleanup_helper(env, &thread3);

    return result;
}
DEFINE_TEST(SCHED0006, "Test IPC priorities for Send", test_ipc_prios)
