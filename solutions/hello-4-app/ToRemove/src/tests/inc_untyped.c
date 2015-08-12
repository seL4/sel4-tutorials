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

#include "../helpers.h"

#ifndef CONFIG_KERNEL_STABLE

static int
test_retype(env_t env, void* args)
{
    int error;
    int i;
    vka_object_t untyped;
    vka_object_t cnode;

    error = vka_alloc_cnode_object(&env->vka, 2, &cnode);
    test_assert(error == 0);

    error = vka_alloc_untyped(&env->vka, seL4_TCBBits + 3, &untyped);
    test_assert(error == 0);

    /* Try to drop two caps in, at the end of the cnode, overrunning it. */
    error = seL4_Untyped_Retype(untyped.cptr,
                                seL4_TCBObject, 0,
                                env->cspace_root, cnode.cptr, seL4_WordBits,
                                (1 << 2) - 1, 2);
    test_assert(error == seL4_RangeError);

    /* Drop some caps in. This should be successful. */
    for (i = 0; i < (1 << 2); i++) {
        error = seL4_Untyped_Retype(untyped.cptr,
                                    seL4_TCBObject, 0,
                                    env->cspace_root, cnode.cptr, seL4_WordBits,
                                    i, 1);
        test_assert(!error);
    }

    /* Try to drop one in beyond the end of the cnode. */
    error = seL4_Untyped_Retype(untyped.cptr,
                                seL4_TCBObject, 0,
                                env->cspace_root, cnode.cptr, seL4_WordBits,
                                i, 2);
    test_assert(error == seL4_RangeError);

    /* Try putting caps over the top. */
    for (i = 0; i < (1 << 2); i++) {
        error = seL4_Untyped_Retype(untyped.cptr,
                                    seL4_TCBObject, 0,
                                    env->cspace_root, cnode.cptr, seL4_WordBits,
                                    i, 1);
        test_assert(error == seL4_DeleteFirst);
    }

    /* Delete them all. */
    for (i = 0; i < (1 << 2); i++) {
        error = seL4_CNode_Delete(cnode.cptr, i, 2);
        test_assert(!error);
    }

    /* Try to insert too many. */
    error = seL4_Untyped_Retype(untyped.cptr,
                                seL4_TCBObject, 0,
                                env->cspace_root, cnode.cptr, seL4_WordBits,
                                0, 1U << 31);
    test_assert(error == seL4_RangeError);

    error = seL4_Untyped_Retype(untyped.cptr,
                                seL4_TCBObject, 0,
                                env->cspace_root, cnode.cptr, seL4_WordBits,
                                0, (1 << 2) + 1);
    test_assert(error == seL4_RangeError);

    /* Insert them in one fell swoop but one. */
    error = seL4_Untyped_Retype(untyped.cptr,
                                seL4_TCBObject, 0,
                                env->cspace_root, cnode.cptr, seL4_WordBits,
                                0, (1 << 2) - 1);
    test_assert(!error);

    /* Try inserting over the top. Only the last should succeed. */
    for (i = 0; i < (1 << 2); i++) {
        error = seL4_Untyped_Retype(untyped.cptr,
                                    seL4_TCBObject, 0,
                                    env->cspace_root, cnode.cptr, seL4_WordBits,
                                    i, 1);
        if (i == (1 << 2) - 1) {
            test_assert(!error);
        } else {
            test_assert(error == seL4_DeleteFirst);
        }
    }

    vka_free_object(&env->vka, &untyped);
    vka_free_object(&env->vka, &cnode);
    return SUCCESS;
}
DEFINE_TEST(RETYPE0000, "Retype test", test_retype)

static int
test_incretype(env_t env, void* args)
{
    int error;
    vka_object_t untyped;
    int size_bits;

    /* Find an untyped of some size. */
    for (size_bits = 13; size_bits > 0; size_bits--) {
        error = vka_alloc_untyped(&env->vka, size_bits, &untyped);
        if (error == 0) {
            break;
        }
    }
    test_assert(error == 0 && untyped.cptr != 0);

    /* Try retyping anything bigger than the object into it. */
    int i;
    for (i = 40; i > 0; i--) {
        error = seL4_Untyped_Retype(untyped.cptr,
                                    seL4_CapTableObject, size_bits - seL4_SlotBits + i,
                                    env->cspace_root, env->cspace_root, seL4_WordBits,
                                    0, 1);
        test_assert(error);
    }

    /* Try retyping an object of the correct size in. */
    error = seL4_Untyped_Retype(untyped.cptr,
                                seL4_CapTableObject, size_bits - seL4_SlotBits + 0,
                                env->cspace_root, env->cspace_root, seL4_WordBits,
                                0, 1);
    test_assert(!error);


    /* clean up */
    vka_free_object(&env->vka, &untyped);

    return SUCCESS;
}
DEFINE_TEST(RETYPE0001, "Incremental retype test", test_incretype)

static int
test_incretype2(env_t env, void* args)
{
    int error;
    seL4_Word slot[17];
    vka_object_t untyped;

    /* Get a bunch of free slots. */
    for (int i = 0; i < sizeof(slot) / sizeof(slot[0]); i++) {
        error = vka_cspace_alloc(&env->vka, &slot[i]);
        test_assert(error == 0);
    }

    /* And an untyped big enough to allocate 16 4-k pages into. */
    error = vka_alloc_untyped(&env->vka, 16, &untyped);
    test_assert(error == 0 && untyped.cptr != 0);

    /* Try allocating precisely 16 pages. These should all work. */
    int i;
    for (i = 0; i < 16; i++) {
        error = seL4_Untyped_Retype(untyped.cptr,
                                    seL4_ARCH_4KPage, 0,
                                    env->cspace_root, env->cspace_root, seL4_WordBits,
                                    slot[i], 1);
        test_assert(!error);
    }

    /* An obscenely large allocation should fail (note that's 2^(2^20)). */
    error = seL4_Untyped_Retype(untyped.cptr,
                                seL4_ARCH_4KPage, 0,
                                env->cspace_root, env->cspace_root, seL4_WordBits,
                                slot[i], 1024 * 1024);
    test_assert(error == seL4_RangeError);

    /* Allocating to an existing slot should fail. */
    error = seL4_Untyped_Retype(untyped.cptr,
                                seL4_ARCH_4KPage, 0,
                                env->cspace_root, env->cspace_root, seL4_WordBits,
                                slot[0], 8);
    test_assert(error == seL4_DeleteFirst);

    /* Allocating another item should also fail as the untyped is full. */
    error = seL4_Untyped_Retype(untyped.cptr,
                                seL4_ARCH_4KPage, 0,
                                env->cspace_root, env->cspace_root, seL4_WordBits,
                                slot[i++], 1);
    test_assert(error == seL4_NotEnoughMemory);


    vka_free_object(&env->vka, &untyped);

    return SUCCESS;
}
DEFINE_TEST(RETYPE0002, "Incremental retype test #2", test_incretype2)

#endif /* !CONFIG_KERNEL_STABLE */
