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
#include <stddef.h>
#include <stdio.h>
#include <sel4/sel4.h>
#include <sel4/messages.h>
#include <vka/object.h>
#include <sel4utils/util.h>

#include "../helpers.h"
#include "frame_type.h"

#if defined(CONFIG_ARCH_ARM) && defined(CONFIG_HAVE_CACHE)
static int
test_page_flush(env_t env, void *args)
{
    seL4_CPtr frame, framec;
    seL4_Word vstart, vstartc;
    volatile uint32_t *ptr, *ptrc;
    vka_t *vka;
    int err;

    vka = &env->vka;

    void *vaddr;
    void *vaddrc;
    reservation_t reservation, reservationc;

    reservation = vspace_reserve_range(&env->vspace,
                                       PAGE_SIZE_4K, seL4_AllRights, 0, &vaddr);
    assert(reservation.res);
    reservationc = vspace_reserve_range(&env->vspace,
                                        PAGE_SIZE_4K, seL4_AllRights, 1, &vaddrc);
    assert(reservationc.res);

    vstart = (uint32_t)vaddr;
    assert(IS_ALIGNED(vstart, seL4_PageBits));
    vstartc = (uint32_t)vaddrc;
    assert(IS_ALIGNED(vstartc, seL4_PageBits));

    ptr = (volatile uint32_t*)vstart;
    ptrc = (volatile uint32_t*)vstartc;

    /* Create a frame */
    frame = vka_alloc_frame_leaky(vka, PAGE_BITS_4K);
    test_assert(frame != seL4_CapNull);

    /* Duplicate the cap */
    framec = get_free_slot(env);
    test_assert(framec != seL4_CapNull);
    err = cnode_copy(env, frame, framec, seL4_AllRights);
    test_assert(!err);

    /* map in a cap with cacheability */
    err = vspace_map_pages_at_vaddr(&env->vspace, &framec, NULL, vaddrc, 1, seL4_PageBits, reservationc);
    test_assert(!err);
    /* map in a cap without cacheability */
    err = vspace_map_pages_at_vaddr(&env->vspace, &frame, NULL, vaddr, 1, seL4_PageBits, reservation);
    test_assert(!err);

    /* Clean makes data observable to non-cached page */
    *ptr = 0xC0FFEE;
    *ptrc = 0xDEADBEEF;
    test_assert(*ptr == 0xC0FFEE);
    test_assert(*ptrc == 0xDEADBEEF);
    err = seL4_ARM_Page_Clean_Data(framec, 0, PAGE_SIZE_4K);
    assert(!err);
    test_assert(*ptr == 0xDEADBEEF);
    test_assert(*ptrc == 0xDEADBEEF);
    /* Clean/Invalidate makes data observable to non-cached page */
    *ptr = 0xC0FFEE;
    *ptrc = 0xDEADBEEF;
    test_assert(*ptr == 0xC0FFEE);
    test_assert(*ptrc == 0xDEADBEEF);
    err = seL4_ARM_Page_CleanInvalidate_Data(framec, 0, PAGE_SIZE_4K);
    assert(!err);
    test_assert(*ptr == 0xDEADBEEF);
    test_assert(*ptrc == 0xDEADBEEF);
    /* Invalidate makes RAM data observable to cached page */
    *ptr = 0xC0FFEE;
    *ptrc = 0xDEADBEEF;
    test_assert(*ptr == 0xC0FFEE);
    test_assert(*ptrc == 0xDEADBEEF);
    err = seL4_ARM_Page_Invalidate_Data(framec, 0, PAGE_SIZE_4K);
    assert(!err);

    /* In case the invalidation performs an implicit clean, write a new
       value to RAM and make sure the cached read retrieves it 
       Remember to drain any store buffer!
    */
    *ptr = 0xBEEFCAFE;
#ifdef CONFIG_ARCH_ARM_V7A
    asm volatile ("dmb st" ::: "memory");
#endif
    test_assert(*ptrc == 0xBEEFCAFE);
    test_assert(*ptr == 0xBEEFCAFE);

    return SUCCESS;
}

static int
test_large_page_flush_operation(env_t env, void *args)
{
    int num_frame_types = ARRAY_SIZE(frame_types);
    seL4_CPtr frames[num_frame_types];
    int error;
    vka_t *vka = &env->vka;
    bool pt_mapped = false;

    /* Grab some free vspace big enough to hold all the tests. */
    seL4_Word vstart;
    reservation_t reserve = vspace_reserve_range(&env->vspace, 2 * (1 << 25),
                                                  seL4_AllRights, 1, (void **) &vstart);
    test_assert(reserve.res != 0);
    vstart = ALIGN_UP(vstart, (1 << 25));

    /* Create us some frames to play with. */
    for (int i = 0; i < num_frame_types; i++) {
        frames[i] = vka_alloc_frame_leaky(vka, CTZ(frame_types[i].size));
        assert(frames[i]);
    }

    /* Also create a pagetable to map the pages into. */
    seL4_CPtr pt = vka_alloc_page_table_leaky(vka);

    /* Map the pages in. */
    for (int i = 0; i < num_frame_types; i++) {
        if (frame_types[i].need_pt && !pt_mapped) {
            /* Map the pagetable in. */
            error = seL4_ARCH_PageTable_Map(pt, env->page_directory,
                                            vstart + frame_types[i].vaddr_offset,
                                            seL4_ARCH_Default_VMAttributes);
            test_assert(error == 0);

            pt_mapped = true;
        }

        error = seL4_ARCH_Page_Map(frames[i], env->page_directory,
                                   vstart + frame_types[i].vaddr_offset, seL4_AllRights,
                                   seL4_ARCH_Default_VMAttributes);
        test_assert(error == 0);
    }

    /* See if we can invoke page flush on each of them */
    for (int i = 0; i < num_frame_types; i++) {
        error = seL4_ARM_Page_Invalidate_Data(frames[i], 0, frame_types[i].size);
        test_assert(error == 0);
        error = seL4_ARM_Page_Clean_Data(frames[i], 0, frame_types[i].size);
        test_assert(error == 0);
        error = seL4_ARM_Page_CleanInvalidate_Data(frames[i], 0, frame_types[i].size);
        test_assert(error == 0);
        error = seL4_ARM_Page_Unify_Instruction(frames[i], 0, frame_types[i].size);
        test_assert(error == 0);
        error = seL4_ARM_PageDirectory_Invalidate_Data(env->page_directory, vstart + frame_types[i].vaddr_offset, vstart + frame_types[i].vaddr_offset + frame_types[i].size);
        test_assert(error == 0);
        error = seL4_ARM_PageDirectory_Clean_Data(env->page_directory, vstart + frame_types[i].vaddr_offset, vstart + frame_types[i].vaddr_offset + frame_types[i].size);
        test_assert(error == 0);
        error = seL4_ARM_PageDirectory_CleanInvalidate_Data(env->page_directory, vstart + frame_types[i].vaddr_offset, vstart + frame_types[i].vaddr_offset + frame_types[i].size);
        test_assert(error == 0);
        error = seL4_ARM_PageDirectory_Unify_Instruction(env->page_directory, vstart + frame_types[i].vaddr_offset, vstart + frame_types[i].vaddr_offset + frame_types[i].size);
        test_assert(error == 0);
    }

    return SUCCESS;
}


static int
test_page_directory_flush(env_t env, void *args)
{
    seL4_CPtr frame, framec;
    seL4_Word vstart, vstartc;
    volatile uint32_t *ptr, *ptrc;
    vka_t *vka;
    int err;

    vka = &env->vka;

    void *vaddr;
    void *vaddrc;
    reservation_t reservation, reservationc;

    reservation = vspace_reserve_range(&env->vspace,
                                       PAGE_SIZE_4K, seL4_AllRights, 0, &vaddr);
    assert(reservation.res);
    reservationc = vspace_reserve_range(&env->vspace,
                                        PAGE_SIZE_4K, seL4_AllRights, 1, &vaddrc);
    assert(reservationc.res);

    vstart = (uint32_t)vaddr;
    assert(IS_ALIGNED(vstart, seL4_PageBits));
    vstartc = (uint32_t)vaddrc;
    assert(IS_ALIGNED(vstartc, seL4_PageBits));

    ptr = (volatile uint32_t*)vstart;
    ptrc = (volatile uint32_t*)vstartc;

    /* Create a frame */
    frame = vka_alloc_frame_leaky(vka, PAGE_BITS_4K);
    test_assert(frame != seL4_CapNull);

    /* Duplicate the cap */
    framec = get_free_slot(env);
    test_assert(framec != seL4_CapNull);
    err = cnode_copy(env, frame, framec, seL4_AllRights);
    test_assert(!err);

    /* map in a cap with cacheability */
    err = vspace_map_pages_at_vaddr(&env->vspace, &framec, NULL, vaddrc, 1, seL4_PageBits, reservationc);
    test_assert(!err);
    /* map in a cap without cacheability */
    err = vspace_map_pages_at_vaddr(&env->vspace, &frame, NULL, vaddr, 1, seL4_PageBits, reservation);
    test_assert(!err);

    /* Clean makes data observable to non-cached page */
    *ptr = 0xC0FFEE;
    *ptrc = 0xDEADBEEF;
    test_assert(*ptr == 0xC0FFEE);
    test_assert(*ptrc == 0xDEADBEEF);
    err = seL4_ARM_PageDirectory_Clean_Data(env->page_directory, vstartc, vstartc + PAGE_SIZE_4K);
    assert(!err);
    test_assert(*ptr == 0xDEADBEEF);
    test_assert(*ptrc == 0xDEADBEEF);
    /* Clean/Invalidate makes data observable to non-cached page */
    *ptr = 0xC0FFEE;
    *ptrc = 0xDEADBEEF;
    test_assert(*ptr == 0xC0FFEE);
    test_assert(*ptrc == 0xDEADBEEF);
    err = seL4_ARM_PageDirectory_CleanInvalidate_Data(env->page_directory, vstartc, vstartc + PAGE_SIZE_4K);
    assert(!err);
    test_assert(*ptr == 0xDEADBEEF);
    test_assert(*ptrc == 0xDEADBEEF);
    /* Invalidate makes RAM data observable to cached page */
    *ptr = 0xC0FFEE;
    *ptrc = 0xDEADBEEF;
    test_assert(*ptr == 0xC0FFEE);
    test_assert(*ptrc == 0xDEADBEEF);
    err = seL4_ARM_PageDirectory_Invalidate_Data(env->page_directory, vstartc, vstartc + PAGE_SIZE_4K);
    assert(!err);
    /* In case the invalidation performs an implicit clean, write a new
       value to RAM and make sure the cached read retrieves it.
       Need to do an invalidate before retrieving though to guard
       against speculative loads */
    *ptr = 0xBEEFCAFE;
    err = seL4_ARM_PageDirectory_Invalidate_Data(env->page_directory, vstartc, vstartc + PAGE_SIZE_4K);
    assert(!err);
    test_assert(*ptrc == 0xBEEFCAFE);
    test_assert(*ptr == 0xBEEFCAFE);

    return SUCCESS;
}

DEFINE_TEST(CACHEFLUSH0001, "Test a cache maintenance on pages", test_page_flush)
DEFINE_TEST(CACHEFLUSH0002, "Test a cache maintenance on page directories", test_page_directory_flush)
DEFINE_TEST(CACHEFLUSH0003, "Test that cache maintenance can be done on large pages", test_large_page_flush_operation)

#endif
