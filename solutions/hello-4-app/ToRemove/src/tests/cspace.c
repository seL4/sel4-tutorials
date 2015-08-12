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

#include "../test.h"
#include "../helpers.h"
#include <utils/util.h>

#define READY_MAGIC 0x12374153
#define SUCCESS_MAGIC 0x12374151



static int
ipc_caller(seL4_Word ep0, seL4_Word ep1, seL4_Word arg3, seL4_Word arg4)
{
    /* Let our parent know we are ready. */
    seL4_MessageInfo_t tag = seL4_MessageInfo_new(0, 0, 0, 1);
    seL4_SetMR(0, READY_MAGIC);
    seL4_Send(ep0, tag);
    /*
     * The parent has changed our cspace on us. Check that it makes sense.
     *
     * Basically the entire cspace should be empty except for the cap at ep0.
     * We should still test that various points in the cspace resolve correctly.
     */

    /* Check that none of the typical endpoints are valid. */
    for (int i = 0; i < 32; i++) {
        seL4_MessageInfo_ptr_new(&tag, 0, 0, 0, 0);
        tag = seL4_Call(i, tag);
        test_assert(seL4_MessageInfo_get_label(tag) == seL4_InvalidCapability);
    }

    /* Check that changing one bit still gives an invalid cap. */
    for (int i = 0; i < 32; i++) {
        seL4_MessageInfo_ptr_new(&tag, 0, 0, 0, 0);
        tag = seL4_Call(ep1 ^ (1 << i), tag);
        test_assert(seL4_MessageInfo_get_label(tag) == seL4_InvalidCapability);
    }

    /* And we're done. This should be a valid cap and get us out of here! */
    seL4_MessageInfo_ptr_new(&tag, 0, 0, 0, 1);
    seL4_SetMR(0, SUCCESS_MAGIC);
    seL4_Send(ep1, tag);

    printf("%d\n", __LINE__);
    return SUCCESS;
}

static int
test_32bit_cspace(env_t env, void *args)
{
    int error;
    seL4_CPtr cnode[32];
    seL4_CPtr ep = vka_alloc_endpoint_leaky(&env->vka);

    /* Create 32 cnodes, each resolving one bit. */
    for (int i = 0; i < 32; i++) {
        cnode[i] = vka_alloc_cnode_object_leaky(&env->vka, 1);
        assert(cnode[i]);
    }

    /* Copy cnode i to alternating slots in cnode i-1. */
    int slot = 0;
    for (int i = 1; i < 32; i++) {
        error = seL4_CNode_Copy(
                    cnode[i - 1], slot, 1,
                    env->cspace_root, cnode[i], seL4_WordBits,
                    seL4_AllRights);
        test_assert(!error);

        slot ^= 1;
    }

    /* In the final cnode, put an IPC endpoint in slot 1. */
    error = seL4_CNode_Copy(
                cnode[31], slot, 1,
                env->cspace_root, ep, seL4_WordBits,
                seL4_AllRights);
    test_assert(!error);

    /* Start a helper thread in our own cspace, to let it get set up. */
    helper_thread_t t;

    create_helper_thread(env, &t);
    start_helper(env, &t, ipc_caller, ep, 0x55555555, 0, 0);

    /* Wait for it. */
    seL4_MessageInfo_t tag;
    seL4_Word sender_badge = 0;
    tag = seL4_Wait(ep, &sender_badge);
    test_assert(seL4_MessageInfo_get_length(tag) == 1);
    test_assert(seL4_GetMR(0) == READY_MAGIC);

    /* Now switch its cspace. */
    error = seL4_TCB_SetSpace(t.thread.tcb.cptr, t.fault_endpoint,
                              cnode[0], seL4_NilData, env->page_directory,
                              seL4_NilData);

    test_assert(!error);

    /* And now wait for it to do some tests and return to us. */
    tag = seL4_Wait(ep, &sender_badge);
    test_assert(seL4_MessageInfo_get_length(tag) == 1);
    test_assert(seL4_GetMR(0) == SUCCESS_MAGIC);

    cleanup_helper(env, &t);
    return SUCCESS;
}
DEFINE_TEST(CSPACE0001, "Test full 32-bit cspace resolution", test_32bit_cspace)
