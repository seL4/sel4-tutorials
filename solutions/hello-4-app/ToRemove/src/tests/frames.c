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
#include <stdlib.h>

#include <sel4/sel4.h>
#include <sel4/messages.h>
#include <vka/object.h>
#include <sel4utils/util.h>
#include <sel4utils/mapping.h>

#include "../helpers.h"
#include "frame_type.h"

static void
fill_memory(seL4_Word addr, seL4_Word size_bytes)
{
    test_assert_fatal(IS_ALIGNED(addr, sizeof(seL4_Word)));
    test_assert_fatal(IS_ALIGNED(size_bytes, sizeof(seL4_Word)));
    seL4_Word *p = (void*)addr;
    seL4_Word size_words = size_bytes / sizeof(seL4_Word);
    while (size_words--) {
        *p++ = size_words ^ 0xbeefcafe;
    }
}

static int
test_frame_recycle(env_t env, void *args)
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
    test_assert(vstart != 0);
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

        /* Fill each of these with random junk and then recycle them. */
        fill_memory(
            vstart + frame_types[i].vaddr_offset,
            frame_types[i].size);
    }

    /* Now recycle all of them. */
    for (int i = 0; i < num_frame_types; i++) {
        error = seL4_CNode_Recycle(env->cspace_root,
                                   frames[i], seL4_WordBits);
        test_assert(!error);
    }

    /* Map them back in again. */
    for (int i = 0; i < num_frame_types; i++) {
        error = seL4_ARCH_Page_Map(frames[i], env->page_directory,
                                   vstart + frame_types[i].vaddr_offset, seL4_AllRights,
                                   seL4_ARCH_Default_VMAttributes);
        test_assert(error == 0);
    }

    /* Now check they are zero. */
    for (int i = 0; i < num_frame_types; i++) {
        test_assert(check_zeroes(vstart + frame_types[i].vaddr_offset,
                                 frame_types[i].size));
    }

    vspace_free_reservation(&env->vspace, reserve);

    return SUCCESS;
}
DEFINE_TEST(FRAMERECYCLE0001, "Test recycling of frame caps", test_frame_recycle)

static int
test_frame_exported(env_t env, void *args)
{
    struct frame_table_entry {
        vka_object_t frame;
        vka_object_t pt;
        char is_ft;
    };

    vka_t *vka;

    struct frame_table_entry* ft;
    seL4_Word ft_index;
    seL4_Word ft_bytes;

    seL4_Word vaddr_scratch;
    seL4_Word mem_total;
    int err;
    int i;

    vka = &env->vka;


    /* Initialise a lazily allocated frame table */
    ft_index = 0;
    ft_bytes = (1 << (seL4_WordBits - seL4_PageBits)) * sizeof(*ft);

    reservation_t ft_reservation = vspace_reserve_range(&env->vspace,
                                                         ft_bytes + (1 << frame_types[0].size), seL4_AllRights,
                                                         1, (void **) &ft);
    test_assert(ft_reservation.res != NULL);
    ft = (struct frame_table_entry *) ALIGN_UP((unsigned int) ft, frame_types[0].size);

    /* Reserve some vspace for mapping frames */
    reservation_t vaddr_reserve = vspace_reserve_range(&env->vspace, frame_types[0].size * 2,
                                                        seL4_AllRights, 1, (void **) &vaddr_scratch);

    vaddr_scratch = ALIGN_UP(vaddr_scratch, frame_types[0].size);

    test_assert(vaddr_scratch != 0);
    test_assert(vaddr_reserve.res != NULL);

    /* loop through frame sizes, allocate, map and touch them until we run out
     * of memory. */
    mem_total = 0;
    for (i = 0; i < ARRAY_SIZE(frame_types); i++) {
        while (1) {
            vka_object_t pt = {0};
            vka_object_t frame = {0};
            seL4_CPtr vaddr;
            char* data;
            /* Allocate the frame */
            err = vka_alloc_frame(&env->vka, CTZ(frame_types[i].size), &frame);

            if (err) {
                break;
            }

            /* Decide if the frame will be used for the frame table */
            if (mem_total < ft_bytes) {
                vaddr = (seL4_Word)ft + mem_total;
            } else {
                vaddr = vaddr_scratch;
            }
            mem_total += frame_types[i].size;
            /* Map in the page */
            err = seL4_ARCH_Page_Map(frame.cptr,
                                     env->page_directory,
                                     vaddr,
                                     seL4_AllRights,
                                     seL4_ARCH_Default_VMAttributes);
            /* Retry with a page table */
            if (err == seL4_FailedLookup) {
                /* Map in a page table */
                err = vka_alloc_page_table(vka, &pt);

                test_assert(pt.cptr != seL4_CapNull);
                err = seL4_ARCH_PageTable_Map(pt.cptr,
                                              env->page_directory,
                                              vaddr,
                                              seL4_ARCH_Default_VMAttributes);
                test_assert(!err);
                err = seL4_ARCH_Page_Map(frame.cptr,
                                         env->page_directory,
                                         vaddr,
                                         seL4_AllRights,
                                         seL4_ARCH_Default_VMAttributes);
                test_assert(!err);
            }

            test_assert(!err);
            /* Touch the memory */
            data = (char*)vaddr;

            *data = 'U';
            test_assert(*data == 'U');

#ifndef CONFIG_KERNEL_STABLE
            err = seL4_ARCH_Page_Remap(frame.cptr,
                                       env->page_directory,
                                       seL4_AllRights,
                                       seL4_ARCH_Default_VMAttributes);
            test_assert(!err);
            /* Touch the memory again */
            *data = 'V';
            test_assert(*data == 'V');
#endif

            /* Unmap the page if it is not used for our frame table */
            if (vaddr == vaddr_scratch) {
                err = seL4_ARCH_Page_Unmap(frame.cptr);
            }

            /* Update the frame table. This must be done last because
             * it may be backed by the frame that we just tested. */
            ft[ft_index].frame = frame;
            ft[ft_index].pt = pt;
            ft[ft_index].is_ft = (vaddr != vaddr_scratch);
            ft_index++;

        }
    }

    /* clean up -- backwards, so we don't step on our own feet */
    for (int i = 0; i < ft_index; i++) {
        if (!ft[i].is_ft) {
            vka_free_object(&env->vka, &ft[i].frame);
            if (ft[i].pt.cptr != 0) {
                vka_free_object(vka, &ft[i].pt);
            }
        }
    }

    vspace_free_reservation(&env->vspace, vaddr_reserve);
    vspace_free_reservation(&env->vspace, ft_reservation);
    return SUCCESS;
}
DEFINE_TEST(FRAMEEXPORTS0001, "Test that we can access all exported frames", test_frame_exported)

#if defined(CONFIG_ARCH_ARM) && defined(CONFIG_KERNEL_BRANCH_XN)
/* XN support is only implemented for ARM currently. */

/* Function that generates a fault. If we're mapped XN we should instruction
 * fault at the start of the function. If not we should data fault on 0x42.
 */
static void fault(void)
{
    *(char*)0x42 = 'c';
}

/* Wait for a VM fault originating on the given EP the return the virtual
 * address it occurred at. Returns the sentinel 0xffffffff if the message
 * received was not a VM fault.
 */
static void *handle(seL4_CPtr fault_ep)
{
    seL4_MessageInfo_t info = seL4_Wait(fault_ep, NULL);
    if (seL4_MessageInfo_get_label(info) == seL4_VMFault) {
        return (void*)seL4_GetMR(1);
    } else {
        return (void*)0xffffffff;
    }
}

static int test_xn(env_t env, void *args, seL4_ArchObjectType frame_type)
{

    /* Find the size of the frame type we want to test. */
    size_t sz = 0;
    for (unsigned int i = 0; i < sizeof(frame_types) / sizeof(frame_types[0]); i++) {
        if (frame_types[i].type == frame_type) {
            sz = frame_types[i].size;
            break;
        }
    }
    test_assert(sz != 0);

    /* Get ourselves a frame. */
    seL4_CPtr frame_cap = vka_alloc_frame_leaky(&env->vka, CTZ(sz));
    test_assert(frame_cap != seL4_CapNull);

    /* Get ourselves a place to map it. */
    void *dest = (void*)vmem_alloc(sz, sz);
    test_assert(dest != NULL);

    /* First map the frame without the XN bit set to confirm we can execute in
     * it.
     */
    bool pt_mapped = false;
    int err;
retry:
    err = seL4_ARM_Page_Map(frame_cap, env->page_directory, (seL4_Word)dest,
                            seL4_AllRights, seL4_ARM_Default_VMAttributes);
    if (err == seL4_FailedLookup && !pt_mapped) {
        /* We're missing a covering page table. */
        seL4_CPtr pt_cap = vka_alloc_page_table_leaky(&env->vka);
        test_assert(pt_cap != seL4_CapNull);
        err = seL4_ARM_PageTable_Map(pt_cap, env->page_directory, (seL4_Word)dest,
                                     seL4_ARM_Default_VMAttributes);
        test_assert(err == 0);
        pt_mapped = true;
        goto retry;
    } else if (err != 0) {
        test_assert(!"unexpected failure when mapping page");
    }

    assert(err == 0);

    /* Set up a function we're going to have another thread call. Assume that
     * the function is no more than 100 bytes long.
     */
    memcpy(dest, (void*)fault, 100);

    /* First setup a fault endpoint.
     */
    seL4_CPtr fault_ep = vka_alloc_endpoint_leaky(&env->vka);
    test_assert(fault_ep != seL4_CapNull);

    /* Then setup the thread that will, itself, fault. */
    helper_thread_t faulter;
    create_helper_thread(&faulter, &env->vka, (helper_func_t)dest, 0, 0, 0, 0, 0);
    set_helper_thread_name(&faulter, "faulter");
    err = seL4_TCB_Configure(faulter.tcb, fault_ep, 100,
                             env->cspace_root, seL4_NilData, env->page_directory,
                             seL4_NilData, faulter.ipc_buffer_vaddr, faulter.ipc_buffer_frame);
    test_assert(err == 0);

    /* Now a fault handler that will catch and diagnose its fault. */
    helper_thread_t handler;
    create_helper_thread(&handler, &env->vka, (helper_func_t)handle, 0, fault_ep, 0, 0, 0);
    set_helper_thread_name(&handler, "handler");

    /* OK, go. */
    start_helper_thread(&handler);
    start_helper_thread(&faulter);
    void *res = (void*)wait_for_thread(&handler);

    test_assert(res == (void*)0x42);

    cleanup_helper_thread(&handler);
    cleanup_helper_thread(&faulter);

    /* Now let's remap the page with XN set and confirm that we can't execute
     * in it any more.
     */
    err = seL4_ARM_Page_Remap(frame_cap, env->page_directory, seL4_AllRights,
                              seL4_ARM_Default_VMAttributes | seL4_ARM_ExecuteNever);
    assert(err == 0);

    /* The page should still contain our code from before. */
    assert(!memcmp(dest, (void*)fault, 100));

    /* We need to reallocate a fault EP because the thread cleanup above
     * inadvertently destroys the EP we were using.
     */
    fault_ep = vka_alloc_endpoint_leaky(&env->vka);
    test_assert(fault_ep != seL4_CapNull);

    /* Recreate our two threads. */
    create_helper_thread(&faulter, &env->vka, (helper_func_t)dest, 0, 0, 0, 0, 0);
    set_helper_thread_name(&faulter, "faulter");
    err = seL4_TCB_Configure(faulter.tcb, fault_ep, 100,
                             env->cspace_root, seL4_NilData, env->page_directory,
                             seL4_NilData, faulter.ipc_buffer_vaddr, faulter.ipc_buffer_frame);

    create_helper_thread(&handler, &env->vka, (helper_func_t)handle, 0, fault_ep, 0, 0, 0);
    set_helper_thread_name(&handler, "handler");

    /* Try the execution again. */
    start_helper_thread(&handler);
    start_helper_thread(&faulter);
    res = (void*)wait_for_thread(&handler);

    /* Confirm that, this time, we faulted at the start of the XN-mapped page. */
    test_assert(res == (void*)dest);

    /* Resource tear down. */
    cleanup_helper_thread(&handler);
    cleanup_helper_thread(&faulter);
    err = seL4_ARM_Page_Unmap(frame_cap);
    assert(err == 0);
    vmem_free((seL4_Word)dest);

    return SUCCESS;
}

static int test_xn_small_frame(env_t env, void *args)
{
    return test_xn(env, args, seL4_ARM_SmallPageObject);
}
DEFINE_TEST(FRAMEXN0001, "Test that we can map a small frame XN", test_xn_small_frame)

static int test_xn_large_frame(env_t env, void *args)
{
    return test_xn(env, args, seL4_ARM_LargePageObject);
}
DEFINE_TEST(FRAMEXN0002, "Test that we can map a large frame XN", test_xn_large_frame)

#endif
