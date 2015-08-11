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
#include <stdlib.h>
#include <sel4/sel4.h>
#include <vka/object.h>

#include "../helpers.h"

#define MIN_LENGTH 0
#define MAX_LENGTH (seL4_MsgMaxLength + 1)

#define FOR_EACH_LENGTH(len_var) \
    for(int len_var = MIN_LENGTH; len_var <= MAX_LENGTH; len_var++)

typedef int (*test_func_t)(seL4_Word /* endpoint */, seL4_Word /* seed */, seL4_Word /* extra */);

static int
send_func(seL4_Word endpoint, seL4_Word seed, seL4_Word arg2)
{
    FOR_EACH_LENGTH(length) {
        seL4_MessageInfo_t tag = seL4_MessageInfo_new(0, 0, 0, length);
        for (int i = 0; i < length; i++) {
            seL4_SetMR(i, seed);
            seed++;
        }
        seL4_Send(endpoint, tag);
    }

    return SUCCESS;
}

static int
nbsend_func(seL4_Word endpoint, seL4_Word seed, seL4_Word arg2)
{
    FOR_EACH_LENGTH(length) {
        seL4_MessageInfo_t tag = seL4_MessageInfo_new(0, 0, 0, length);
        for (int i = 0; i < length; i++) {
            seL4_SetMR(i, seed);
            seed++;
        }
        seL4_NBSend(endpoint, tag);
    }

    return SUCCESS;
}

static int
call_func(seL4_Word endpoint, seL4_Word seed, seL4_Word arg2)
{
    FOR_EACH_LENGTH(length) {
        seL4_MessageInfo_t tag = seL4_MessageInfo_new(0, 0, 0, length);

        /* Construct a message. */
        for (int i = 0; i < length; i++) {
            seL4_SetMR(i, seed);
            seed++;
        }

        tag = seL4_Call(endpoint, tag);

        seL4_Word actual_len = length;
        /* Sanity check the received message. */
        if (actual_len <= seL4_MsgMaxLength) {
            test_assert(seL4_MessageInfo_get_length(tag) == actual_len);
        } else {
            actual_len = seL4_MsgMaxLength;
        }

        for (int i = 0; i < actual_len; i++) {
            seL4_Word mr = seL4_GetMR(i);
            test_check(mr == seed);
            seed++;
        }
    }

    return SUCCESS;
}

static int
wait_func(seL4_Word endpoint, seL4_Word seed, seL4_Word arg2)
{
    FOR_EACH_LENGTH(length) {
        seL4_MessageInfo_t tag;
        seL4_Word sender_badge = 0;

        tag = seL4_Wait(endpoint, &sender_badge);
        seL4_Word actual_len = length;
        if (actual_len <= seL4_MsgMaxLength) {
            test_assert(seL4_MessageInfo_get_length(tag) == actual_len);
        } else {
            actual_len = seL4_MsgMaxLength;
        }

        for (int i = 0; i < actual_len; i++) {
            seL4_Word mr = seL4_GetMR(i);
            test_check(mr == seed);
            seed++;
        }
    }

    return SUCCESS;
}

static int
nbwait_func(seL4_Word endpoint, seL4_Word seed, seL4_Word nbwait_should_wait)
{
    if (!nbwait_should_wait) {
        return SUCCESS;
    }

    FOR_EACH_LENGTH(length) {
        seL4_MessageInfo_t tag;
        seL4_Word sender_badge = 0;

        tag = seL4_Wait(endpoint, &sender_badge);
        seL4_Word actual_len = length;
        if (actual_len <= seL4_MsgMaxLength) {
            test_assert(seL4_MessageInfo_get_length(tag) == actual_len);
        } else {
            actual_len = seL4_MsgMaxLength;
        }

        for (int i = 0; i < actual_len; i++) {
            seL4_Word mr = seL4_GetMR(i);
            test_check(mr == seed);
            seed++;
        }
    }

    return SUCCESS;
}

static int
replywait_func(seL4_Word endpoint, seL4_Word seed, seL4_Word arg2)
{
    int first = 1;

    seL4_MessageInfo_t tag;
    FOR_EACH_LENGTH(length) {
        seL4_Word sender_badge = 0;

        /* First reply/wait can't reply. */
        if (first) {
            tag = seL4_Wait(endpoint, &sender_badge);
            first = 0;
        } else {
            tag = seL4_ReplyWait(endpoint, tag, &sender_badge);
        }

        seL4_Word actual_len = length;
        /* Sanity check the received message. */
        if (actual_len <= seL4_MsgMaxLength) {
            test_assert(seL4_MessageInfo_get_length(tag) == actual_len);
        } else {
            actual_len = seL4_MsgMaxLength;
        }

        for (int i = 0; i < actual_len; i++) {
            seL4_Word mr = seL4_GetMR(i);
            test_check(mr == seed);
            seed++;
        }
        /* Seed will have changed more if the message was truncated. */
        for (int i = actual_len; i < length; i++) {
            seed++;
        }

        /* Construct a reply. */
        for (int i = 0; i < actual_len; i++) {
            seL4_SetMR(i, seed);
            seed++;
        }
    }

    /* Need to do one last reply to match call. */
    seL4_Reply(tag);

    return SUCCESS;
}

static int
reply_and_wait_func(seL4_Word endpoint, seL4_Word seed, seL4_Word arg2)
{
    int first = 1;

    seL4_MessageInfo_t tag;
    FOR_EACH_LENGTH(length) {
        seL4_Word sender_badge = 0;

        /* First reply/wait can't reply. */
        if (!first) {
            seL4_Reply(tag);
        } else {
            first = 0;
        }

        tag = seL4_Wait(endpoint, &sender_badge);

        seL4_Word actual_len = length;
        /* Sanity check the received message. */
        if (actual_len <= seL4_MsgMaxLength) {
            test_assert(seL4_MessageInfo_get_length(tag) == actual_len);
        } else {
            actual_len = seL4_MsgMaxLength;
        }

        for (int i = 0; i < actual_len; i++) {
            seL4_Word mr = seL4_GetMR(i);
            test_check(mr == seed);
            seed++;
        }
        /* Seed will have changed more if the message was truncated. */
        for (int i = actual_len; i < length; i++) {
            seed++;
        }

        /* Construct a reply. */
        for (int i = 0; i < actual_len; i++) {
            seL4_SetMR(i, seed);
            seed++;
        }
    }

    /* Need to do one last reply to match call. */
    seL4_Reply(tag);

    return SUCCESS;
}

static int
test_ipc_pair(env_t env, test_func_t fa, test_func_t fb, bool inter_as)
{
    helper_thread_t thread_a, thread_b;
    vka_t *vka = &env->vka;

    int error;
    seL4_CPtr ep = vka_alloc_endpoint_leaky(vka);
    seL4_Word start_number = 0xabbacafe;

    /* Test sending messages of varying lengths. */
    /* Please excuse the awful indending here. */
    for (int sender_prio = 98; sender_prio <= 102; sender_prio++) {
        for (int waiter_prio = 100; waiter_prio <= 100; waiter_prio++) {
            for (int sender_first = 0; sender_first <= 1; sender_first++) {
#if 0
                fprintf(stdout, "%d %s %d\n",
                        sender_prio, sender_first ? "->" : "<-", waiter_prio);
#endif
                seL4_Word thread_a_arg0, thread_b_arg0;

                if (inter_as) {
                    create_helper_process(env, &thread_a);

                    cspacepath_t path;
                    vka_cspace_make_path(&env->vka, ep, &path);
                    thread_a_arg0 = sel4utils_copy_cap_to_process(&thread_a.process, path);
                    assert(thread_a_arg0 != -1);

                    create_helper_process(env, &thread_b);
                    thread_b_arg0 = sel4utils_copy_cap_to_process(&thread_b.process, path);
                    assert(thread_b_arg0 != -1);

                } else {
                    create_helper_thread(env, &thread_a);
                    create_helper_thread(env, &thread_b);
                    thread_a_arg0 = ep;
                    thread_b_arg0 = ep;
                }

                set_helper_priority(&thread_a, sender_prio);
                set_helper_priority(&thread_b, waiter_prio);

                /* Set the flag for nbwait_func that tells it whether or not it really
                 * should wait. */
                int nbwait_should_wait;
                nbwait_should_wait =
                    (sender_prio < waiter_prio);

                /* Threads are enqueued at the head of the scheduling queue, so the
                 * thread enqueued last will be run first, for a given priority. */
                if (sender_first) {
                    start_helper(env, &thread_b, (helper_fn_t) fb, thread_b_arg0, start_number,
                                 nbwait_should_wait, 0);
                    start_helper(env, &thread_a, (helper_fn_t) fa, thread_a_arg0, start_number,
                                 nbwait_should_wait, 0);
                } else {
                    start_helper(env, &thread_a, (helper_fn_t) fa, thread_a_arg0, start_number,
                                 nbwait_should_wait, 0);
                    start_helper(env, &thread_b, (helper_fn_t) fb, thread_b_arg0, start_number,
                                 nbwait_should_wait, 0);
                }

                wait_for_helper(&thread_a);
                wait_for_helper(&thread_b);

                cleanup_helper(env, &thread_a);
                cleanup_helper(env, &thread_b);

                start_number += 0x71717171;
            }
        }
    }

    error = cnode_delete(env, ep);
    test_assert(!error);
    return SUCCESS;
}

static int
test_send_wait(env_t env, void *args)
{
    return test_ipc_pair(env, send_func, wait_func, false);
}
DEFINE_TEST(IPC0001, "Test seL4_Send + seL4_Wait", test_send_wait)

static int
test_call_replywait(env_t env, void *args)
{
    return test_ipc_pair(env, call_func, replywait_func, false);
}
DEFINE_TEST(IPC0002, "Test seL4_Call + seL4_ReplyWait", test_call_replywait)

static int
test_call_reply_and_wait(env_t env, void *args)
{
    return test_ipc_pair(env, call_func, reply_and_wait_func, false);
}
DEFINE_TEST(IPC0003, "Test seL4_Send + seL4_Reply + seL4_Wait", test_call_reply_and_wait)

static int
test_nbsend_wait(env_t env, void *args)
{
    return test_ipc_pair(env, nbsend_func, nbwait_func, false);
}
DEFINE_TEST(IPC0004, "Test seL4_NBSend + seL4_Wait", test_nbsend_wait)

static int
test_send_wait_interas(env_t env, void *args)
{
    return test_ipc_pair(env, send_func, wait_func, true);
}
DEFINE_TEST(IPC1001, "Test inter-AS seL4_Send + seL4_Wait", test_send_wait_interas)

static int
test_call_replywait_interas(env_t env, void *args)
{
    return test_ipc_pair(env, call_func, replywait_func, true);
}
DEFINE_TEST(IPC1002, "Test inter-AS seL4_Call + seL4_ReplyWait", test_call_replywait_interas)

static int
test_call_reply_and_wait_interas(env_t env, void *args)
{
    return test_ipc_pair(env, call_func, reply_and_wait_func, true);
}
DEFINE_TEST(IPC1003, "Test inter-AS seL4_Send + seL4_Reply + seL4_Wait", test_call_reply_and_wait_interas)

static int
test_nbsend_wait_interas(env_t env, void *args)
{
    return test_ipc_pair(env, nbsend_func, nbwait_func, true);
}
DEFINE_TEST(IPC1004, "Test inter-AS seL4_NBSend + seL4_Wait", test_nbsend_wait_interas)

static int
test_ipc_abort_in_call(env_t env, void *args)
{
    helper_thread_t thread_a;
    vka_t * vka = &env->vka;

    seL4_CPtr ep = vka_alloc_endpoint_leaky(vka);

    seL4_Word start_number = 0xabbacafe;

    create_helper_thread(env, &thread_a);
    set_helper_priority(&thread_a, 100);

    start_helper(env, &thread_a, (helper_fn_t) call_func, ep, start_number, 0, 0);

    /* Wait for the endpoint that it's going to call. */
    seL4_Word sender_badge = 0;

    seL4_Wait(ep, &sender_badge);

    /* Now suspend the thread. */
    seL4_TCB_Suspend(thread_a.thread.tcb.cptr);

    /* Now resume the thread. */
    seL4_TCB_Resume(thread_a.thread.tcb.cptr);

    /* Now suspend it again for good measure. */
    seL4_TCB_Suspend(thread_a.thread.tcb.cptr);

    /* And delete it. */
    cleanup_helper(env, &thread_a);

    return SUCCESS;
}
DEFINE_TEST(IPC0010, "Test suspending an IPC mid-Call()", test_ipc_abort_in_call)
