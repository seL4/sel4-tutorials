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

#ifdef CONFIG_AEP_BINDING
#include "../helpers.h"

#define NUM_RUNS 10
#define ASYNC 1
#define SYNC 2

static seL4_CPtr
badge_endpoint(env_t env, int badge, seL4_CPtr ep)
{

    seL4_CapData_t cap_data = seL4_CapData_Badge_new(badge);
    seL4_CPtr slot = get_free_slot(env);
    int error = cnode_mint(env, ep, slot, seL4_AllRights, cap_data);
    test_assert(!error);
    return slot;
}

static int
sender(seL4_Word ep, seL4_Word id, seL4_Word runs, seL4_Word arg3)
{
    assert(runs > 0);
    for (int i = 0; i < runs; i++) {
        seL4_MessageInfo_t info = seL4_MessageInfo_new(0, 0, 0, 0);
        seL4_Send((seL4_CPtr) ep, info);
    }

    return 0;
}

static int
test_aep_binding(env_t env, void* args)
{
    helper_thread_t sync, async;

    /* create endpoints */
    seL4_CPtr sync_ep = vka_alloc_endpoint_leaky(&env->vka);
    seL4_CPtr async_ep = vka_alloc_async_endpoint_leaky(&env->vka);
    seL4_CPtr badged_async_ep = badge_endpoint(env, ASYNC, async_ep);
    seL4_CPtr badged_sync_ep = badge_endpoint(env, SYNC, sync_ep);

    assert(async_ep);
    assert(sync_ep);

    /* badge endpoints so we can tell them apart */
    create_helper_thread(env, &sync);
    create_helper_thread(env, &async);

    /* bind the endpoint */
    int error = seL4_TCB_BindAEP(env->tcb, async_ep);
    test_assert(error == seL4_NoError);

    start_helper(env, &async, sender, badged_async_ep, ASYNC, NUM_RUNS, 0);
    start_helper(env, &sync, sender, badged_sync_ep, SYNC, NUM_RUNS, 0);

    int num_async_messages = 0;
    int num_sync_messages = 0;
    for (int i = 0; i < NUM_RUNS * 2; i++) {
        seL4_Word badge = 0;
        seL4_Wait(sync_ep, &badge);

        switch (badge) {
        case ASYNC:
            num_async_messages++;
            break;
        case SYNC:
            num_sync_messages++;
            break;
        }
    }

    test_check(num_async_messages == NUM_RUNS);
    test_check(num_sync_messages == NUM_RUNS);

    error = seL4_TCB_UnbindAEP(env->tcb);
    test_assert(error == seL4_NoError);

    cleanup_helper(env, &sync);
    cleanup_helper(env, &async);

    return SUCCESS;
}
DEFINE_TEST(BIND0001, "Test that a bound tcb waiting on a sync endpoint receives normal sync ipc and async notifications.", test_aep_binding)


static int
test_aep_binding_2(env_t env, void* args)
{
    helper_thread_t async;

    /* create endpoints */
    seL4_CPtr async_ep = vka_alloc_async_endpoint_leaky(&env->vka);
    seL4_CPtr badged_async_ep = badge_endpoint(env, ASYNC, async_ep);

    test_assert(async_ep);
    test_assert(badged_async_ep);

    /* badge endpoints so we can tell them apart */
    create_helper_thread(env, &async);

    /* bind the endpoint */
    int error = seL4_TCB_BindAEP(env->tcb, async_ep);
    test_assert(error == seL4_NoError);

    start_helper(env, &async, sender, badged_async_ep, ASYNC, NUM_RUNS, 0);

    int num_async_messages = 0;
    for (int i = 0; i < NUM_RUNS; i++) {
        seL4_Word badge = 0;
        seL4_Wait(async_ep, &badge);

        switch (badge) {
        case ASYNC:
            num_async_messages++;
            break;
        }
    }

    test_check(num_async_messages == NUM_RUNS);

    error = seL4_TCB_UnbindAEP(env->tcb);
    test_assert(error == seL4_NoError);

    cleanup_helper(env, &async);

    return SUCCESS;
}
DEFINE_TEST(BIND0002, "Test that a bound tcb waiting on its bound endpoint recieves notifications", test_aep_binding_2)

/* helper thread for testing the ordering of bound async endpoint operations */
static int
waiter(seL4_Word bound_ep, seL4_Word arg1, seL4_Word arg2, seL4_Word arg3)
{
    seL4_Word badge;
    seL4_Wait(bound_ep, &badge);
    return 0;
}

static int
test_aep_binding_prio(env_t env, uint8_t waiter_prio, uint8_t sender_prio)
{
    helper_thread_t waiter_thread;
    helper_thread_t sender_thread;

    seL4_CPtr async_ep = vka_alloc_async_endpoint_leaky(&env->vka);
    seL4_CPtr sync_ep = vka_alloc_endpoint_leaky(&env->vka);

    test_assert(async_ep);
    test_assert(sync_ep);

    create_helper_thread(env, &waiter_thread);
    set_helper_priority(&waiter_thread, waiter_prio);

    create_helper_thread(env, &sender_thread);
    set_helper_priority(&sender_thread, sender_prio);

    int error = seL4_TCB_BindAEP(waiter_thread.thread.tcb.cptr, async_ep);
    test_assert(error == seL4_NoError);

    start_helper(env, &waiter_thread, waiter, async_ep, 0, 0, 0);
    start_helper(env, &sender_thread, sender, async_ep, 0, 1, 0);

    wait_for_helper(&waiter_thread);
    wait_for_helper(&sender_thread);

    cleanup_helper(env, &waiter_thread);
    cleanup_helper(env, &sender_thread);

    return SUCCESS;
}

static int
test_aep_binding_3(env_t env, void* args)
{
    test_aep_binding_prio(env, 10, 9);
    return SUCCESS;
}
DEFINE_TEST(BIND0003, "Test IPC ordering 1) bound tcb waits on bound endpoint 2) another tcb sends a message",
            test_aep_binding_3)

static int
test_aep_binding_4(env_t env, void* args)
{
    test_aep_binding_prio(env, 9, 10);
    return SUCCESS;
}
DEFINE_TEST(BIND0004, "Test IPC ordering 2) bound tcb waits on bound endpoint 1) another tcb sends a message",
            test_aep_binding_4)

#endif /* CONFIG_AEP_BINDING */
