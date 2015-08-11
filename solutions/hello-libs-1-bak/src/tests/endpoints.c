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
#include <vka/object.h>

#include "../test.h"
#include "../helpers.h"

#define NUM_BADGED_CLIENTS 32

static int
bouncer_func(seL4_CPtr ep, seL4_Word arg1, seL4_Word arg2, seL4_Word arg3)
{
    seL4_MessageInfo_t tag = seL4_MessageInfo_new(0, 0, 0, 0);
    seL4_Word sender_badge;
    seL4_Wait(ep, &sender_badge);
    while (1) {
        seL4_ReplyWait(ep, tag, &sender_badge);
    }
    return 0;
}

static int
call_func(seL4_CPtr ep, seL4_Word msg, volatile seL4_Word *done, seL4_Word arg3)
{
    seL4_MessageInfo_t tag = seL4_MessageInfo_new(0, 0, 0, 1);

    /* Send the given message once. */
    seL4_SetMR(0, msg);
    tag = seL4_Call(ep, tag);
    test_check(seL4_MessageInfo_get_length(tag) == 1);
    test_check(seL4_GetMR(0) == ~msg);

    *done = 0;

    /* Send the given message again - should (eventually) fault this time. */
    seL4_SetMR(0, msg);
    tag = seL4_Call(ep, tag);
    /* The call should fail. */
    test_check(seL4_MessageInfo_get_label(tag) == seL4_InvalidCapability);

    *done = 1;

    return SUCCESS;
}

static int
test_ep_recycle(env_t env, void* args)
{
    seL4_MessageInfo_t tag = seL4_MessageInfo_new(0, 0, 0, 0);
    struct {
        helper_thread_t thread;
        seL4_CPtr badged_ep;
        seL4_CPtr derived_badged_ep;
        volatile seL4_Word done;
    } senders[NUM_BADGED_CLIENTS];
    helper_thread_t bouncer;
    seL4_CPtr bounce_ep;
    UNUSED int error;
    seL4_CPtr ep;

    /* Create the master endpoint. */
    ep = vka_alloc_endpoint_leaky(&env->vka);

    /* Create N badged endpoints, and derive each of them. */
    for (int i = 0; i < NUM_BADGED_CLIENTS; i++) {
        senders[i].badged_ep = get_free_slot(env);
        assert(senders[i].badged_ep != 0);

        senders[i].derived_badged_ep = get_free_slot(env);
        assert(senders[i].derived_badged_ep != 0);

        seL4_CapData_t cap_data;
        cap_data = seL4_CapData_Badge_new (i + 200);
        error = cnode_mint(env, ep, senders[i].badged_ep, seL4_AllRights, cap_data);
        assert(!error);

        error = cnode_copy(env, senders[i].badged_ep, senders[i].derived_badged_ep, seL4_AllRights);
        assert(!error);

        create_helper_thread(env, &senders[i].thread);
        set_helper_priority(&senders[i].thread, 100);

        senders[i].done = -1;
    }
    /* Create a bounce thread so we can get lower prio threads to run. */
    bounce_ep = vka_alloc_endpoint_leaky(&env->vka);
    create_helper_thread(env, &bouncer);
    set_helper_priority(&bouncer, 0);
    start_helper(env, &bouncer, bouncer_func, bounce_ep, 0, 0, 0);

    for (int i = 0; i < NUM_BADGED_CLIENTS; i++) {
        start_helper(env, &senders[i].thread, (helper_fn_t) call_func,
                     senders[i].derived_badged_ep, i + 100, (seL4_Word) &senders[i].done, 0);
    }

    /* Let the sender threads run. */
    seL4_Call(bounce_ep, tag);
    /* Receive a message from each endpoint and check the badge. */
    for (int i = 0; i < NUM_BADGED_CLIENTS; i++) {
        seL4_Word sender_badge;
        seL4_MessageInfo_ptr_set_length(&tag, 1);
        tag = seL4_Wait(ep, &sender_badge);
        assert(seL4_MessageInfo_get_length(tag) == 1);
        assert(seL4_GetMR(0) == sender_badge - 100);
        seL4_SetMR(0, ~seL4_GetMR(0));
        seL4_Reply(tag);
    }
    /* Let the sender threads run. */
    seL4_Call(bounce_ep, tag);
    /* Check none of the threads have failed yet. */
    for (int i = 0; i < NUM_BADGED_CLIENTS; i++) {
        assert(senders[i].done == 0);
    }
    /* Recycle each endpoint. */
    for (int i = 0; i < NUM_BADGED_CLIENTS; i++) {
        error = cnode_recycle(env, senders[i].badged_ep);
        assert(!error);

        /* Let thread run. */
        seL4_Call(bounce_ep, tag);
        /* Check that only the intended threads have now aborted. */
        for (int j = 0; j < NUM_BADGED_CLIENTS; j++) {
            if (j <= i) {
                assert(senders[j].done == 1);
            } else {
                assert(senders[j].done == 0);
            }
        }
    }
    seL4_Call(bounce_ep, tag);
    for (int i = 0; i < NUM_BADGED_CLIENTS; i++) {
        cleanup_helper(env, &senders[i].thread);
    }
    cleanup_helper(env, &bouncer);

    return SUCCESS;
}
DEFINE_TEST(RECYCLE0001, "Basic endpoint recycle testing.", test_ep_recycle)

static int ep_test_func(seL4_CPtr sync_ep, seL4_CPtr test_ep, volatile int *status, seL4_Word arg4)
{
    seL4_MessageInfo_t tag = seL4_MessageInfo_new(0, 0, 0, 0);
    seL4_Word sender_badge;
    while (1) {
        seL4_Wait(sync_ep, &sender_badge);
        /* Hit up the test end point */
        seL4_MessageInfo_t reply = seL4_Call(test_ep, tag);
        /* See what the status was */
        *status = !!(seL4_MessageInfo_get_label(reply) != seL4_InvalidCapability);
        /* Reply */
        seL4_Reply(tag);
    }
    return SUCCESS;
}

/* RECYCLE0001 only tests if a thread gets its IPC canceled. The IPC
 * can succeed even if the cap it used got deleted provided the final
 * capability was not recycled (thus causing an IPC cancel to happen)
 * This means that RECYCLE0001 will pass even if all the derived badges
 * are deleted, since deleting them did NOT delete the final capability
 * or cause a recycle so outstanding IPCs were not canceled */
static int
test_ep_recycle2(env_t env, void* args)
{
    seL4_MessageInfo_t tag = seL4_MessageInfo_new(0, 0, 0, 0);
    struct {
        helper_thread_t thread;
        seL4_CPtr sync_ep;
        seL4_CPtr badged_ep;
        seL4_CPtr derived_badged_ep;
        volatile seL4_Word last_status;
    } helpers[NUM_BADGED_CLIENTS];
    seL4_CPtr ep;
    helper_thread_t reply_thread;
    UNUSED int error;

    /* Create the main EP we are testing */
    ep = vka_alloc_endpoint_leaky(&env->vka);
    /* spawn a thread to keep replying to any messages on main ep */
    create_helper_thread(env, &reply_thread);
    start_helper(env, &reply_thread, bouncer_func, ep, 0, 0, 0);
    /* Spawn helper threads each with their own sync ep and a badged copy of the main ep */
    for (int i = 0; i < NUM_BADGED_CLIENTS; i++) {
        helpers[i].badged_ep = get_free_slot(env);
        assert(helpers[i].badged_ep != 0);

        helpers[i].derived_badged_ep = get_free_slot(env);
        assert(helpers[i].derived_badged_ep != 0);

        helpers[i].sync_ep = vka_alloc_endpoint_leaky(&env->vka);

        seL4_CapData_t cap_data;
        cap_data = seL4_CapData_Badge_new (i + 200);
        error = cnode_mint(env, ep, helpers[i].badged_ep, seL4_AllRights, cap_data);
        assert(!error);

        error = cnode_copy(env, helpers[i].badged_ep, helpers[i].derived_badged_ep, seL4_AllRights);
        assert(!error);

        helpers[i].last_status = -1;

        create_helper_thread(env, &helpers[i].thread);
        start_helper(env, &helpers[i].thread, (helper_fn_t) ep_test_func,
                     helpers[i].sync_ep, helpers[i].derived_badged_ep, (seL4_Word) &helpers[i].last_status, 0);
    }
    /* Test that every thread and endpoint is working */
    for (int i = 0; i < NUM_BADGED_CLIENTS; i++) {
        seL4_Call(helpers[i].sync_ep, tag);
    }
    for (int i = 0; i < NUM_BADGED_CLIENTS; i++) {
        test_assert(helpers[i].last_status);
    }
    /* Now start recycling endpoints and make sure the correct endpoints disappear */
    for (int i = 0 ; i < NUM_BADGED_CLIENTS; i++) {
        /* Recycle an ep */
        error = cnode_recycle(env, helpers[i].badged_ep);
        assert(!error);
        /* Now run every thread */
        for (int j = 0; j < NUM_BADGED_CLIENTS; j++) {
            seL4_Call(helpers[j].sync_ep, tag);
        }
        for (int j = 0; j < NUM_BADGED_CLIENTS; j++) {
            if (j <= i) {
                test_assert(!helpers[j].last_status);
            } else {
                test_assert(helpers[j].last_status);
            }
        }
    }
    return SUCCESS;
}
DEFINE_TEST(RECYCLE0002, "Recycle deletes caps", test_ep_recycle2)
