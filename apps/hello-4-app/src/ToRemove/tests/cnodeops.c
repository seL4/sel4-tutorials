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
#include <sel4/sel4.h>
#include <vka/vka.h>
#include <vka/object.h>

#include "../test.h"
#include "../helpers.h"

/* Get a cap that we can move/copy/delete/etc and compare without causing chaos.
 */
static seL4_CPtr
get_cap(vka_t *vka)
{
    return vka_alloc_tcb_leaky(vka);
}
static int
test_cnode_copy(env_t env, void* args)
{
    int error;
    seL4_Word src, dest;

    /* A call that should succeed. */
    src = get_cap(&env->vka);
    dest = get_free_slot(env);
    test_assert(is_slot_empty(env, dest));
    error = cnode_copy(env, src, dest, seL4_AllRights);
    test_assert(!error);
    test_assert(!are_tcbs_distinct(src, dest));

    /* Copy to an occupied slot (should fail). */
    src = get_cap(&env->vka);
    dest = get_cap(&env->vka);
    error = cnode_copy(env, src, dest, seL4_AllRights);
    test_assert(error == seL4_DeleteFirst);
    test_assert(are_tcbs_distinct(src, dest));

    /* Copy from a free slot to an occupied slot (should fail). */
    src = get_free_slot(env);
    test_assert(is_slot_empty(env, src));
    dest = get_cap(&env->vka);
    error = cnode_copy(env, src, dest, seL4_AllRights);
    test_assert(error == seL4_DeleteFirst);
    test_assert(are_tcbs_distinct(src, dest));

    /* Copy from a free slot to a free slot (should fail). */
    src = get_free_slot(env);
    test_assert(is_slot_empty(env, src));
    dest = get_free_slot(env);
    test_assert(is_slot_empty(env, dest));
    error = cnode_copy(env, src, dest, seL4_AllRights);
    test_assert(error == seL4_FailedLookup);
    test_assert(are_tcbs_distinct(src, dest));

    return SUCCESS;
}
DEFINE_TEST(CNODEOP0001, "Basic seL4_CNode_Copy() testing", test_cnode_copy)

static int
test_cnode_delete(env_t env, void* args)
{
    int error;
    seL4_Word slot;

    /* A call that should succeed. */
    slot = get_cap(&env->vka);
    error = cnode_delete(env, slot);
    test_assert(!error);
    test_assert(is_slot_empty(env, slot));

    /* Deleting a free slot (should succeed). */
    slot = get_free_slot(env);
    error = cnode_delete(env, slot);
    test_assert(!error);
    test_assert(is_slot_empty(env, slot));

    return SUCCESS;
}
DEFINE_TEST(CNODEOP0002, "Basic seL4_CNode_Delete() testing", test_cnode_delete)

static int
test_cnode_mint(env_t env, void* args)
{
    int error;
    seL4_Word src, dest;

    /* A call that should succeed. */
    src = get_cap(&env->vka);
    dest = get_free_slot(env);
    error = cnode_mint(env, src, dest, seL4_AllRights, seL4_NilData);
    test_assert(!error);
    test_assert(!are_tcbs_distinct(src, dest));

    /* Mint to an occupied slot (should fail). */
    src = get_cap(&env->vka);
    dest = get_cap(&env->vka);
    error = cnode_mint(env, src, dest, seL4_AllRights, seL4_NilData);
    test_assert(error == seL4_DeleteFirst);
    test_assert(are_tcbs_distinct(src, dest));

    /* Mint from an empty slot (should fail). */
    src = get_free_slot(env);
    dest = get_free_slot(env);
    error = cnode_mint(env, src, dest, seL4_AllRights, seL4_NilData);
    test_assert(error == seL4_FailedLookup);
    test_assert(are_tcbs_distinct(src, dest));

    return SUCCESS;
}
DEFINE_TEST(CNODEOP0003, "Basic seL4_CNode_Mint() testing", test_cnode_mint)

static int
test_cnode_move(env_t env, void* args)
{
    int error;
    seL4_Word src, dest;

    /* A call that should succeed. */
    src = get_cap(&env->vka);
    dest = get_free_slot(env);
    error = cnode_move(env, src, dest);
    test_assert(!error);
    test_assert(is_slot_empty(env, src));
    test_assert(!is_slot_empty(env, dest));

    /* Move from an empty slot (should fail). */
    src = get_free_slot(env);
    dest = get_free_slot(env);
    error = cnode_move(env, src, dest);
    test_assert(error = seL4_FailedLookup);
    test_assert(is_slot_empty(env, dest));

    /* Move to an occupied slot (should fail). */
    src = get_cap(&env->vka);
    dest = get_cap(&env->vka);
    error = cnode_move(env, src, dest);
    test_assert(error == seL4_DeleteFirst);
    test_assert(!is_slot_empty(env, src));
    test_assert(!is_slot_empty(env, dest));

    return SUCCESS;
}
DEFINE_TEST(CNODEOP0004, "Basic seL4_CNode_Move() testing", test_cnode_move)

static int
test_cnode_mutate(env_t env, void* args)
{
    int error;
    seL4_Word src, dest;

    /* A call that should succeed. */
    src = get_cap(&env->vka);
    dest = get_free_slot(env);
    error = cnode_mutate(env, src, dest);
    test_assert(!error);
    test_assert(is_slot_empty(env, src));
    test_assert(!is_slot_empty(env, dest));

    /* Mutating to an occupied slot (should fail). */
    src = get_cap(&env->vka);
    dest = get_cap(&env->vka);
    error = cnode_mutate(env, src, dest);
    test_assert(error == seL4_DeleteFirst);
    test_assert(!is_slot_empty(env, src));
    test_assert(!is_slot_empty(env, dest));

    /* Mutating an empty slot (should fail). */
    src = get_free_slot(env);
    dest = get_free_slot(env);
    error = cnode_mutate(env, src, dest);
    test_assert(error == seL4_FailedLookup);
    test_assert(is_slot_empty(env, src));
    test_assert(is_slot_empty(env, dest));

    return SUCCESS;
}
DEFINE_TEST(CNODEOP0005, "Basic seL4_CNode_Mutate() testing", test_cnode_mutate)

static int
test_cnode_recycle(env_t env, void* args)
{
    int error;
    seL4_Word slot;

    /* A call that should succeed. */
    slot = get_cap(&env->vka);
    error = cnode_recycle(env, slot);
    test_assert(!error);
    test_assert(!is_slot_empty(env, slot));

    /* Recycling an empty slot (should fail). */
    slot = get_free_slot(env);
    error = cnode_recycle(env, slot);
    test_assert(error == seL4_IllegalOperation);
    test_assert(is_slot_empty(env, slot));

    return SUCCESS;
}
DEFINE_TEST(CNODEOP0006, "Basic seL4_CNode_Recycle() testing", test_cnode_recycle)

static int
test_cnode_revoke(env_t env, void* args)
{
    int error;
    seL4_Word slot;

    /* A call that should succeed. */
    slot = get_cap(&env->vka);
    error = cnode_revoke(env, slot);
    test_assert(!error);
    test_assert(!is_slot_empty(env, slot));

    /* Revoking a null cap (should fail). */
    slot = get_free_slot(env);
    error = cnode_revoke(env, slot);
    test_assert(!error);
    test_assert(is_slot_empty(env, slot));

    return SUCCESS;
}
DEFINE_TEST(CNODEOP0007, "Basic seL4_CNode_Revoke() testing", test_cnode_revoke)

static int
test_cnode_rotate(env_t env, void* args)
{
    int error;
    seL4_Word src, pivot, dest;

    /* A call that should succeed. */
    src = get_cap(&env->vka);
    pivot = get_cap(&env->vka);
    dest = get_free_slot(env);
    error = cnode_rotate(env, src, pivot, dest);
    test_assert(!error);
    test_assert(is_slot_empty(env, src));
    test_assert(!is_slot_empty(env, pivot));
    test_assert(!is_slot_empty(env, dest));

    /* Destination occupied (should fail). */
    src = get_cap(&env->vka);
    pivot = get_cap(&env->vka);
    dest = get_cap(&env->vka);
    error = cnode_rotate(env, src, pivot, dest);
    test_assert(error == seL4_DeleteFirst);
    test_assert(!is_slot_empty(env, src));
    test_assert(!is_slot_empty(env, pivot));
    test_assert(!is_slot_empty(env, dest));

    /* Swapping two caps (should succeed). */
    src = get_cap(&env->vka);
    pivot = get_cap(&env->vka);
    dest = src;
    error = cnode_rotate(env, src, pivot, dest);
    test_assert(!error);
    test_assert(!are_tcbs_distinct(src, dest));
    test_assert(!is_slot_empty(env, pivot));

    /* Moving a cap onto itself (should fail). */
    src = get_cap(&env->vka);
    pivot = src;
    dest = get_free_slot(env);
    error = cnode_rotate(env, src, pivot, dest);
    test_assert(error == seL4_IllegalOperation);
    test_assert(!is_slot_empty(env, src));
    test_assert(is_slot_empty(env, dest));

    /* Moving empty slots (should fail). */
    src = get_free_slot(env);
    pivot = get_free_slot(env);
    dest = get_free_slot(env);
    error = cnode_rotate(env, src, pivot, dest);
    test_assert(error == seL4_FailedLookup);
    test_assert(is_slot_empty(env, src));
    test_assert(is_slot_empty(env, pivot));
    test_assert(is_slot_empty(env, dest));

    return SUCCESS;
}
DEFINE_TEST(CNODEOP0008, "Basic seL4_CNode_Rotate() testing", test_cnode_rotate)

static int
test_cnode_savecaller(env_t env, void* args)
{
    int error;
    seL4_Word slot;

    /* A call that should succeed. */
    slot = get_free_slot(env);
    error = cnode_savecaller(env, slot);
    test_assert(!error);

    /* Save to an occupied slot (should fail). */
    slot = get_cap(&env->vka);
    error = cnode_savecaller(env, slot);
    test_assert(error == seL4_DeleteFirst);
    test_assert(!is_slot_empty(env, slot));

    /* TODO: Test saving an actual reply capability. */

    return SUCCESS;
}
DEFINE_TEST(CNODEOP0009, "Basic seL4_CNode_SaveCaller() testing", test_cnode_savecaller)
