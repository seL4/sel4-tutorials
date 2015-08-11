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
#include <vka/object.h>

#include "../helpers.h"

#if defined(ARCH_ARM)
#define PAGE_SIZE    (1 << (vka_get_object_size(seL4_ARM_SmallPageObject, 0)))
#define LPAGE_SIZE   (1 << (vka_get_object_size(seL4_ARM_LargePageObject, 0)))
#define SECT_SIZE    (1 << (vka_get_object_size(seL4_ARM_SectionObject, 0)))
#define SUPSECT_SIZE (1 << (vka_get_object_size(seL4_ARM_SuperSectionObject, 0)))

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
__attribute__((warn_unused_result))
check_memory(seL4_Word addr, seL4_Word size_bytes)
{
    test_assert_fatal(IS_ALIGNED(addr, sizeof(seL4_Word)));
    test_assert_fatal(IS_ALIGNED(size_bytes, sizeof(seL4_Word)));
    seL4_Word *p = (void*)addr;
    seL4_Word size_words = size_bytes / sizeof(seL4_Word);
    while (size_words--) {
        if (*p++ != (size_words ^ 0xbeefcafe)) {
            return 0;
        }
    }
    return 1;
}

static int
test_pagetable_arm(env_t env, void *args)
{
    int error;

    /* Grab some free vspace big enough to hold a couple of supersections. */
    seL4_Word vstart;
    reservation_t reserve = vspace_reserve_range(&env->vspace, SUPSECT_SIZE * 4,
                                                  seL4_AllRights, 1, (void **) &vstart);
    vstart = ALIGN_UP(vstart, SUPSECT_SIZE * 2);


    /* Create us some frames to play with. */
    seL4_CPtr small_page = vka_alloc_object_leaky(&env->vka, seL4_ARM_SmallPageObject, 0);
    seL4_CPtr small_page2 = vka_alloc_object_leaky(&env->vka, seL4_ARM_SmallPageObject, 0);
    seL4_CPtr large_page = vka_alloc_object_leaky(&env->vka, seL4_ARM_LargePageObject, 0);
    seL4_CPtr section = vka_alloc_object_leaky(&env->vka, seL4_ARM_SectionObject, 0);
    seL4_CPtr supersection = vka_alloc_object_leaky(&env->vka, seL4_ARM_SuperSectionObject, 0);

    test_assert(small_page != 0);
    test_assert(small_page2 != 0);
    test_assert(large_page != 0);
    test_assert(section != 0);
    test_assert(supersection != 0);

    /* Also create a pagetable to map the pages into. */
    seL4_CPtr pt = vka_alloc_page_table_leaky(&env->vka);

    /* Check we can't map the supersection in at an address it's not aligned
     * to. */
    test_assert_fatal(supersection != 0);
    error = seL4_ARM_Page_Map(supersection, env->page_directory,
                              vstart + SUPSECT_SIZE / 2, seL4_AllRights, seL4_ARM_Default_VMAttributes);
    test_assert(error == seL4_AlignmentError);

    /* Check we can map it in somewhere correctly aligned. */
    error = seL4_ARM_Page_Map(supersection, env->page_directory,
                              vstart + SUPSECT_SIZE, seL4_AllRights, seL4_ARM_Default_VMAttributes);
    test_assert(error == 0);

    /* Now fill it with stuff to check later. */
    /* TDDO fx these constants */
    fill_memory(vstart + SUPSECT_SIZE, SUPSECT_SIZE);

    /* Check we now can't map a section over the top. */
    error = seL4_ARM_Page_Map(section, env->page_directory,
                              vstart + SUPSECT_SIZE + SUPSECT_SIZE / 2, seL4_AllRights,
                              seL4_ARM_Default_VMAttributes);
    test_assert(error == seL4_DeleteFirst);

    /* Unmapping the section shouldn't do anything. */
    error = seL4_ARM_Page_Unmap(section);
    test_assert(error == 0);

    /* Unmap supersection and try again. */
    error = seL4_ARM_Page_Unmap(supersection);
    test_assert(error == 0);

    error = seL4_ARM_Page_Map(section, env->page_directory,
                              vstart + SUPSECT_SIZE + SUPSECT_SIZE / 2, seL4_AllRights,
                              seL4_ARM_Default_VMAttributes);
    test_assert(error == 0);

    fill_memory(vstart + SUPSECT_SIZE + SUPSECT_SIZE / 2, SECT_SIZE);

    /* Now that a section is there, see if we can map a supersection over the
     * top. */
    error = seL4_ARM_Page_Map(supersection, env->page_directory,
                              vstart + SUPSECT_SIZE, seL4_AllRights, seL4_ARM_Default_VMAttributes);
    test_assert(error == seL4_DeleteFirst);

    if (!check_memory(vstart + SUPSECT_SIZE + SUPSECT_SIZE / 2, SECT_SIZE)) {
        return FAILURE;
    }

    /* Unmap the section, leaving nothing mapped. */
    error = seL4_ARM_Page_Unmap(section);
    test_assert(error == 0);

    /* Now, try mapping in the supersection into two places. */
    error = seL4_ARM_Page_Map(supersection, env->page_directory,
                              vstart, seL4_AllRights, seL4_ARM_Default_VMAttributes);
    test_assert(error == 0);
    error = seL4_ARM_Page_Map(supersection, env->page_directory,
                              vstart + SUPSECT_SIZE, seL4_AllRights, seL4_ARM_Default_VMAttributes);
    test_assert(error == seL4_InvalidCapability);

    /* Now check what we'd written earlier is still there. */
    test_assert(check_memory(vstart, SUPSECT_SIZE));

    /* Try mapping the section into two places. */
    error = seL4_ARM_Page_Map(section, env->page_directory,
                              vstart + 2 * SUPSECT_SIZE, seL4_AllRights, seL4_ARM_Default_VMAttributes);
    test_assert(error == 0);
    error = seL4_ARM_Page_Map(section, env->page_directory,
                              vstart + SUPSECT_SIZE, seL4_AllRights, seL4_ARM_Default_VMAttributes);
    test_assert(error == seL4_InvalidCapability);

    /* Unmap everything again. */
    error = seL4_ARM_Page_Unmap(section);
    test_assert(error == 0);
    error = seL4_ARM_Page_Unmap(supersection);
    test_assert(error == 0);

    /* Map a large page somewhere with no pagetable. */
    error = seL4_ARM_Page_Map(large_page, env->page_directory,
                              vstart, seL4_AllRights, seL4_ARM_Default_VMAttributes);
    test_assert(error == seL4_FailedLookup);

    /* Map a pagetable at an unaligned address. Oddly enough, this will succeed
     * as the kernel silently truncates the address to the nearest correct
     * boundary. */
    error = seL4_ARM_PageTable_Map(pt, env->page_directory,
                                   vstart + SECT_SIZE + 10, seL4_ARM_Default_VMAttributes);
    test_assert(error == 0);

    /* Map the large page in at an unaligned address. */
    error = seL4_ARM_Page_Map(large_page, env->page_directory,
                              vstart + SECT_SIZE + LPAGE_SIZE / 2,
                              seL4_AllRights, seL4_ARM_Default_VMAttributes);
    test_assert(error == seL4_AlignmentError);

    /* Map the large page in at an aligned address. */
    error = seL4_ARM_Page_Map(large_page, env->page_directory,
                              vstart + SECT_SIZE + LPAGE_SIZE,
                              seL4_AllRights, seL4_ARM_Default_VMAttributes);
    test_assert(error == 0);

    /* Map it again, and it should fail. */
    error = seL4_ARM_Page_Map(large_page, env->page_directory,
                              vstart + SECT_SIZE + 2 * LPAGE_SIZE,
                              seL4_AllRights, seL4_ARM_Default_VMAttributes);
    test_assert(error == seL4_InvalidCapability);

    /* Fill it with more stuff to check. */
    fill_memory(vstart + SECT_SIZE + LPAGE_SIZE, LPAGE_SIZE);

    /* Try mapping a small page over the top of it. */
    error = seL4_ARM_Page_Map(small_page, env->page_directory,
                              vstart + SECT_SIZE + LPAGE_SIZE,
                              seL4_AllRights, seL4_ARM_Default_VMAttributes);
    test_assert(error == seL4_DeleteFirst);

    /* Try mapping a small page elsewhere useful. */
    error = seL4_ARM_Page_Map(small_page, env->page_directory,
                              vstart + SECT_SIZE + 3 * LPAGE_SIZE,
                              seL4_AllRights, seL4_ARM_Default_VMAttributes);
    test_assert(error == 0);

    /* Fill the small page with useful data too. */
    fill_memory(vstart + SECT_SIZE + 3 * LPAGE_SIZE, PAGE_SIZE);

    /* Pull the plug on the page table. Apparently a recycle isn't good enough.
     * Get another pagetable! */
    error = seL4_CNode_Delete(env->cspace_root, pt, seL4_WordBits);
    test_assert(error == 0);
    error = seL4_ARM_Page_Unmap(small_page);
    test_assert(error == 0);
    error = seL4_ARM_Page_Unmap(large_page);
    test_assert(error == 0);
    pt = vka_alloc_page_table_leaky(&env->vka);

    /* Map the pagetable somewhere new. */
    error = seL4_ARM_PageTable_Map(pt, env->page_directory,
                                   vstart, seL4_ARM_Default_VMAttributes);
    test_assert(error == 0);

    /* Map our small page and large page back in. */
    error = seL4_ARM_Page_Map(small_page, env->page_directory,
                              vstart + PAGE_SIZE,
                              seL4_AllRights, seL4_ARM_Default_VMAttributes);
    test_assert(error == 0);

    error = seL4_ARM_Page_Map(large_page, env->page_directory,
                              vstart + LPAGE_SIZE,
                              seL4_AllRights, seL4_ARM_Default_VMAttributes);
    test_assert(error == 0);

    /* Check their contents. */
    test_assert(check_memory(vstart + PAGE_SIZE, PAGE_SIZE));
    test_assert(check_memory(vstart + LPAGE_SIZE, LPAGE_SIZE));

    /* Recycle the small page. */
    error = seL4_CNode_Recycle(env->cspace_root, small_page,
                               seL4_WordBits);
    test_assert(error == 0);

    /* Unmap the large page. */
    error = seL4_ARM_Page_Unmap(large_page);
    test_assert(error == 0);

    /* Now map the large page where the small page was. */
    error = seL4_ARM_Page_Map(large_page, env->page_directory,
                              vstart,
                              seL4_AllRights, seL4_ARM_Default_VMAttributes);
    test_assert(error == 0);

    /* Now unmap the small page. This should have no effect as we've recycled
     * it already. */
    error = seL4_ARM_Page_Unmap(small_page);
    test_assert(error == 0);

    /* Now check the contents of the large page. */
    test_assert(check_memory(vstart, LPAGE_SIZE));

    /* Map the small page elsewhere. */
    error = seL4_ARM_Page_Map(small_page, env->page_directory,
                              vstart + LPAGE_SIZE,
                              seL4_AllRights, seL4_ARM_Default_VMAttributes);
    test_assert(error == 0);

    /* And the small page should be filled with zeroes. */
    test_assert(check_zeroes(vstart + LPAGE_SIZE, PAGE_SIZE));

    /* Recycle it. */
    error = seL4_CNode_Recycle(env->cspace_root, small_page,
                               seL4_WordBits);
    test_assert(error == 0);

    /* Map a different small page in its place. */
    error = seL4_ARM_Page_Map(small_page2, env->page_directory,
                              vstart + LPAGE_SIZE,
                              seL4_AllRights, seL4_ARM_Default_VMAttributes);
    test_assert(error == 0);

    /* Fill it in with stuff. */
    fill_memory(vstart + LPAGE_SIZE, PAGE_SIZE);

    /* Now unmap the original page. */
    error = seL4_ARM_Page_Unmap(small_page);

    /* Should still be able to access the new page. */
    test_assert(check_memory(vstart + LPAGE_SIZE, PAGE_SIZE));

    vspace_free_reservation(&env->vspace, reserve);

    return SUCCESS;
}
DEFINE_TEST(PT0001, "Fun with page tables on ARM", test_pagetable_arm)

#if 0
/* This test is commented out while we wait for the bugfix */

/* Assumes caps can be mapped in at vaddr (= [vaddr,vaddr + 3*size) */
static int
do_test_pagetable_tlbflush_on_vaddr_reuse(seL4_CPtr cap1, seL4_CPtr cap2, seL4_Word vstart,
                                          seL4_Word size)
{
    int error;
    seL4_Word vaddr;
    volatile seL4_Word* vptr = NULL;


    /* map, touch page 1 */
    vaddr = vstart;
    vptr = (seL4_Word*)vaddr;
    error = seL4_ARM_Page_Map(cap1, env->page_directory,
                              vaddr, seL4_AllRights,
                              seL4_ARM_Default_VMAttributes);
    test_assert(error == 0);
    vptr[0] = 1;
    error = seL4_ARM_Page_Unmap(cap1);
    test_assert(error == 0);

    /* map, touch page 2 */
    vaddr = vstart + size;
    vptr = (seL4_Word*)vaddr;
    error = seL4_ARM_Page_Map(cap2, env->page_directory,
                              vaddr, seL4_AllRights,
                              seL4_ARM_Default_VMAttributes);
    test_assert(error == 0);
    vptr[0] = 2;
    error = seL4_ARM_Page_Unmap(cap2);
    test_assert(error == 0);

    /* Test TLB */
    vaddr = vstart + 2 * size;
    vptr = (seL4_Word*)vaddr;
    error = seL4_ARM_Page_Map(cap1, env->page_directory,
                              vaddr, seL4_AllRights,
                              seL4_ARM_Default_VMAttributes);
    test_assert(error == 0);
    test_check(vptr[0] == 1);

    error = seL4_ARM_Page_Map(cap2, env->page_directory,
                              vaddr, seL4_AllRights,
                              seL4_ARM_Default_VMAttributes);
    test_assert(error == 0);
    test_check(vptr[0] == 2 || !"TLB contains stale entry");

    /* clean up */
    error = seL4_ARM_Page_Unmap(cap1);
    test_assert(error == 0);
    error = seL4_ARM_Page_Unmap(cap2);
    test_assert(error == 0);
    return SUCCESS;
}

static int
test_pagetable_tlbflush_on_vaddr_reuse(env_t env, void *args)
{
    int error;
    int result = SUCCESS;

    /* Grab some free vspace big enough to hold a couple of supersections. */
    seL4_Word vstart = vmem_alloc(4 * SUPSECT_SIZE, 4 * SUPSECT_SIZE);

    seL4_CPtr cap1, cap2;
    /* Create us some frames to play with. */

    /* Also create a pagetable to map the pages into. */
    seL4_CPtr pt = vka_alloc_page_table_leaky(&env->vka);

    /* supersection */
    cap1 = vka_alloc_object_leaky(&env->vka, seL4_ARM_SuperSectionObject, 0);
    cap2 = vka_alloc_object_leaky(&env->vka, seL4_ARM_SuperSectionObject, 0);
    if (do_test_pagetable_tlbflush_on_vaddr_reuse(cap1, cap2, vstart, SUPSECT_SIZE) == FAILURE) {
        result = FAILURE;
    }
    /* section */
    cap1 = vka_alloc_object_leaky(&env->vka, seL4_ARM_SectionObject, 0);
    cap2 = vka_alloc_object_leaky(&env->vka, seL4_ARM_SectionObject, 0);
    if (do_test_pagetable_tlbflush_on_vaddr_reuse(cap1, cap2, vstart, SUPSECT_SIZE) == FAILURE) {
        result = FAILURE;
    }

    /* map a PT for smaller page objects */
    error = seL4_ARM_PageTable_Map(pt, env->page_directory,
                                   vstart, seL4_ARM_Default_VMAttributes);
    test_assert(error == 0);

    /* Large page */
    cap1 = vka_alloc_object_leaky(&env->vka, seL4_ARM_LargePageObject, 0);
    cap2 = vka_alloc_object_leaky(&env->vka, seL4_ARM_LargePageObject, 0);
    if (do_test_pagetable_tlbflush_on_vaddr_reuse(cap1, cap2, vstart, LPAGE_SIZE) == FAILURE) {
        result = FAILURE;
    }
    /* small page */
    cap1 = vka_alloc_object_leaky(&env->vka, seL4_ARM_SmallPageObject, 0);
    cap2 = vka_alloc_object_leaky(&env->vka, seL4_ARM_SmallPageObject, 0);
    if (do_test_pagetable_tlbflush_on_vaddr_reuse(cap1, cap2, vstart, PAGE_SIZE) == FAILURE) {
        result = FAILURE;
    }

    /* clean up */
    vmem_free(vstart);
    return result;
}
DEFINE_TEST(PT0002, "Reusing virtual addresses flushes TLB", test_pagetable_tlbflush_on_vaddr_reuse)
#endif /* 0 */

#endif /* defined(ARCH_ARM) */
