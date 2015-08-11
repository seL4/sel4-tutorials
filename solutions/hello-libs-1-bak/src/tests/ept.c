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
#include <vka/capops.h>

#include "../test.h"
#include "../helpers.h"

/* Arbitrarily start mapping 128mb into the virtual address range */
#define EPT_MAP_BASE 0x8000000

#define OFFSET_4MB (BIT(22))
#define OFFSET_2MB (OFFSET_4MB >> 1)

/* None of these tests actually check that mappings succeeded. This would require
 * constructing a vcpu, creating a thread and having some code compiled to run
 * inside a vt-x instance. I consider this too much work, and am largely checking
 * that none of these operations will cause the kernel to explode */

#ifdef CONFIG_VTX

static int
map_ept_from_pdpt(env_t env, seL4_CPtr pdpt, seL4_CPtr *pd, seL4_CPtr *pt, seL4_CPtr *frame)
{
    int error;

    *pd = vka_alloc_ept_page_directory_leaky(&env->vka);
    test_assert_fatal(*pd);
    *pt = vka_alloc_ept_page_table_leaky(&env->vka);
    test_assert_fatal(*pt);
    *frame = vka_alloc_frame_leaky(&env->vka, seL4_PageBits);
    test_assert_fatal(*frame);

    error = seL4_IA32_EPTPageDirectory_Map(*pd, pdpt, EPT_MAP_BASE, seL4_IA32_Default_VMAttributes);
    test_assert(error == seL4_NoError);
    error = seL4_IA32_EPTPageTable_Map(*pt, pdpt, EPT_MAP_BASE, seL4_IA32_Default_VMAttributes);
    test_assert(error == seL4_NoError);
    error = seL4_IA32_Page_Map(*frame, pdpt, EPT_MAP_BASE, seL4_AllRights, seL4_IA32_Default_VMAttributes);
    test_assert(error == seL4_NoError);

    return SUCCESS;
}

static int
map_ept_set(env_t env, seL4_CPtr *pdpt, seL4_CPtr *pd, seL4_CPtr *pt, seL4_CPtr *frame)
{
    int error;

    *pdpt = vka_alloc_ept_page_directory_pointer_table_leaky(&env->vka);
    test_assert_fatal(*pdpt);

    error = map_ept_from_pdpt(env, *pdpt, pd, pt, frame);

    return error;
}

static int
map_ept_set_large_from_pdpt(env_t env, seL4_CPtr pdpt, seL4_CPtr *pd, seL4_CPtr *frame)
{
    int error;

    *pd = vka_alloc_ept_page_directory_leaky(&env->vka);
    test_assert_fatal(*pd);
    *frame = vka_alloc_frame_leaky(&env->vka, seL4_LargePageBits);
    test_assert_fatal(*frame);

    error = seL4_IA32_EPTPageDirectory_Map(*pd, pdpt, EPT_MAP_BASE, seL4_IA32_Default_VMAttributes);
    if (error != seL4_NoError) {
        return error;
    }
    error = seL4_IA32_Page_Map(*frame, pdpt, EPT_MAP_BASE, seL4_AllRights, seL4_IA32_Default_VMAttributes);
    if (error != seL4_NoError) {
        return error;
    }

    return SUCCESS;
}

static int
map_ept_set_large(env_t env, seL4_CPtr *pdpt, seL4_CPtr *pd, seL4_CPtr *frame)
{
    int error;

    *pdpt = vka_alloc_ept_page_directory_pointer_table_leaky(&env->vka);
    test_assert_fatal(*pdpt);

    error = map_ept_set_large_from_pdpt(env, *pdpt, pd, frame);

    return error;
}

static int
test_ept_basic_ept(env_t env, void *args)
{
    int error;
    seL4_CPtr pdpt, pd, pt, frame;
    error = map_ept_set(env, &pdpt, &pd, &pt, &frame);
    test_assert(error == seL4_NoError);
    return SUCCESS;
}
DEFINE_TEST(EPT0001, "Testing basic EPT mapping", test_ept_basic_ept)

static int
test_ept_basic_map_unmap(env_t env, void *args)
{
    int error;
    seL4_CPtr pdpt, pd, pt, frame;
    error = map_ept_set(env, &pdpt, &pd, &pt, &frame);
    test_assert(error == seL4_NoError);

    error = seL4_IA32_Page_Unmap(frame);
    test_assert(error == seL4_NoError);
    error = seL4_IA32_EPTPageTable_Unmap(pt);
    test_assert(error == seL4_NoError);
    error = seL4_IA32_EPTPageDirectory_Unmap(pd);
    test_assert(error == seL4_NoError);

    error = map_ept_from_pdpt(env, pdpt, &pd, &pt, &frame);
    test_assert(error == seL4_NoError);

    error = seL4_IA32_EPTPageDirectory_Unmap(pd);
    test_assert(error == seL4_NoError);
    error = seL4_IA32_EPTPageTable_Unmap(pt);
    test_assert(error == seL4_NoError);
    error = seL4_IA32_Page_Unmap(frame);
    test_assert(error == seL4_NoError);

    return SUCCESS;
}
DEFINE_TEST(EPT0002, "Test basic EPT mapping then unmapping", test_ept_basic_map_unmap)

static int
test_ept_basic_map_unmap_large(env_t env, void *args)
{
    int error;
    seL4_CPtr pdpt, pd, frame;
    error = map_ept_set_large(env, &pdpt, &pd, &frame);
    test_assert(error == seL4_NoError);

    error = seL4_IA32_Page_Unmap(frame);
    test_assert(error == seL4_NoError);
    error = seL4_IA32_EPTPageDirectory_Unmap(pd);
    test_assert(error == seL4_NoError);

    error = map_ept_set_large_from_pdpt(env, pdpt, &pd, &frame);
    test_assert(error == seL4_NoError);

    error = seL4_IA32_EPTPageDirectory_Unmap(pd);
    test_assert(error == seL4_NoError);
    error = seL4_IA32_Page_Unmap(frame);
    test_assert(error == seL4_NoError);

    return SUCCESS;
}
DEFINE_TEST(EPT0003, "Test basic EPT mapping then unmapping of large frame", test_ept_basic_map_unmap_large)

static int
test_ept_regression_1(env_t env, void *args)
{
    int error;
    seL4_CPtr pdpt, pd, frame;
    error = map_ept_set_large(env, &pdpt, &pd, &frame);
    test_assert(error == seL4_NoError);

    error = seL4_IA32_Page_Unmap(frame);
    test_assert(error == seL4_NoError);
    error = seL4_IA32_EPTPageDirectory_Unmap(frame);
    test_assert(error != seL4_NoError);

    error = map_ept_set_large_from_pdpt(env, pdpt, &pd, &frame);
    test_assert(error != seL4_NoError);

    return SUCCESS;
}
DEFINE_TEST(EPT1001, "EPT Regression: Unmap large frame then invoke EPT PD unmap on frame", test_ept_regression_1)

static int
test_ept_regression_2(env_t env, void *args)
{
    int error;
    seL4_CPtr pdpt, pd, frame;
    error = map_ept_set_large(env, &pdpt, &pd, &frame);
    test_assert(error == seL4_NoError);

    error = seL4_IA32_Page_Unmap(frame);
    test_assert(error == seL4_NoError);
    error = seL4_IA32_EPTPageDirectory_Unmap(pd);
    test_assert(error == seL4_NoError);

    error = map_ept_set_large_from_pdpt(env, pdpt, &pd, &frame);
    test_assert(error == seL4_NoError);

    error = seL4_IA32_EPTPageDirectory_Unmap(frame);
    test_assert(error != seL4_NoError);
    error = seL4_IA32_Page_Unmap(frame);
    test_assert(error == seL4_NoError);

    return SUCCESS;
}
DEFINE_TEST(EPT1002, "EPT Regression: Invoke EPT PD Unmap on large frame", test_ept_regression_2)

static int
test_ept_no_overlapping_4k(env_t env, void *args)
{
    int error;
    seL4_CPtr pdpt, pd, pt, frame;
    error = map_ept_set(env, &pdpt, &pd, &pt, &frame);
    test_assert(error == seL4_NoError);

    frame = vka_alloc_frame_leaky(&env->vka, seL4_PageBits);
    test_assert_fatal(frame);
    error = seL4_IA32_Page_Map(frame, pdpt, EPT_MAP_BASE, seL4_AllRights, seL4_IA32_Default_VMAttributes);
    test_assert(error != seL4_NoError);
    return SUCCESS;
}
DEFINE_TEST(EPT0004, "Test EPT cannot map overlapping 4k pages", test_ept_no_overlapping_4k)

static int
test_ept_no_overlapping_large(env_t env, void *args)
{
    int error;
    seL4_CPtr pdpt, pd, frame;
    error = map_ept_set_large(env, &pdpt, &pd, &frame);
    test_assert(error == seL4_NoError);

    frame = vka_alloc_frame_leaky(&env->vka, seL4_LargePageBits);
    test_assert_fatal(frame);
    error = seL4_IA32_Page_Map(frame, pdpt, EPT_MAP_BASE, seL4_AllRights, seL4_IA32_Default_VMAttributes);
    test_assert(error != seL4_NoError);
    return SUCCESS;
}
DEFINE_TEST(EPT0005, "Test EPT cannot map overlapping large pages", test_ept_no_overlapping_large)

#ifndef CONFIG_PAE_PAGING
static int
test_ept_aligned_4m(env_t env, void *args)
{
    int error;
    seL4_CPtr pdpt, pd, frame, frame2;
    error = map_ept_set_large(env, &pdpt, &pd, &frame);
    test_assert(error == seL4_NoError);

    frame2 = vka_alloc_frame_leaky(&env->vka, seL4_4MBits);
    test_assert_fatal(frame2);
    /* Try and map a page at +2m */
    error = seL4_IA32_Page_Map(frame2, pdpt, EPT_MAP_BASE + OFFSET_2MB, seL4_AllRights, seL4_IA32_Default_VMAttributes);
    test_assert(error != seL4_NoError);
    /* But we should be able to map it at +4m */
    error = seL4_IA32_Page_Map(frame2, pdpt, EPT_MAP_BASE + OFFSET_4MB, seL4_AllRights, seL4_IA32_Default_VMAttributes);
    test_assert(error == seL4_NoError);

    /* Unmap them both */
    error = seL4_IA32_Page_Unmap(frame);
    test_assert(error == seL4_NoError);
    error = seL4_IA32_Page_Unmap(frame2);
    test_assert(error == seL4_NoError);

    /* Now map the first one at +2m, which should just flat out fail */
    error = seL4_IA32_Page_Map(frame, pdpt, EPT_MAP_BASE + OFFSET_2MB, seL4_AllRights, seL4_IA32_Default_VMAttributes);
    test_assert(error != seL4_NoError);

    return SUCCESS;
}
DEFINE_TEST(EPT0006, "Test EPT 4M mappings must be 4M aligned and cannot overlap", test_ept_aligned_4m)
#endif

#ifndef CONFIG_PAE_PAGING
static int
test_ept_no_overlapping_pt_4m(env_t env, void *args)
{
    int error;
    seL4_CPtr pdpt, pd, pt, frame;
    error = map_ept_set_large(env, &pdpt, &pd, &frame);
    test_assert(error == seL4_NoError);

    pt = vka_alloc_ept_page_table_leaky(&env->vka);
    test_assert_fatal(pt);
    /* now try and map a PT at both 2m entries */
    error = seL4_IA32_EPTPageTable_Map(pt, pdpt, EPT_MAP_BASE, seL4_IA32_Default_VMAttributes);
    test_assert(error != seL4_NoError);
    error = seL4_IA32_EPTPageTable_Map(pt, pdpt, EPT_MAP_BASE + OFFSET_2MB, seL4_IA32_Default_VMAttributes);
    test_assert(error != seL4_NoError);

    /* unmap PT and frame */
    error = seL4_IA32_EPTPageTable_Unmap(pt);
    test_assert(error == seL4_NoError);
    error = seL4_IA32_Page_Unmap(frame);
    test_assert(error == seL4_NoError);

    /* put the page table in this time */
    error = seL4_IA32_EPTPageTable_Map(pt, pdpt, EPT_MAP_BASE, seL4_IA32_Default_VMAttributes);
    test_assert(error == seL4_NoError);
    /* now try the frame */
    error = seL4_IA32_Page_Map(frame, pdpt, EPT_MAP_BASE, seL4_AllRights, seL4_IA32_Default_VMAttributes);
    test_assert(error != seL4_NoError);

    /* unmap the PT */
    error = seL4_IA32_EPTPageTable_Unmap(pt);
    test_assert(error == seL4_NoError);

    /* put the PT and the +2m location, then try the frame */
    error = seL4_IA32_EPTPageTable_Map(pt, pdpt, EPT_MAP_BASE + OFFSET_2MB, seL4_IA32_Default_VMAttributes);
    test_assert(error == seL4_NoError);
    error = seL4_IA32_Page_Map(frame, pdpt, EPT_MAP_BASE, seL4_AllRights, seL4_IA32_Default_VMAttributes);
    test_assert(error != seL4_NoError);

    return SUCCESS;
}
DEFINE_TEST(EPT0007, "Test EPT 4m frame and PT cannot overlap", test_ept_no_overlapping_pt_4m)
#endif

static int
test_ept_map_remap_pd(env_t env, void *args)
{
    int error;
    seL4_CPtr pdpt, pd, pt, frame;
    error = map_ept_set(env, &pdpt, &pd, &pt, &frame);
    test_assert(error == seL4_NoError);

    /* unmap the pd */
    error = seL4_IA32_EPTPageDirectory_Unmap(pd);
    test_assert(error == seL4_NoError);

    /* now map it back in */
    error = seL4_IA32_EPTPageDirectory_Map(pd, pdpt, EPT_MAP_BASE, seL4_IA32_Default_VMAttributes);
    test_assert(error == seL4_NoError);

    /* it should retain its old mappings, and mapping in a new PT should fail */
    pt = vka_alloc_ept_page_table_leaky(&env->vka);
    test_assert_fatal(pt);
    error = seL4_IA32_EPTPageTable_Map(pt, pdpt, EPT_MAP_BASE, seL4_IA32_Default_VMAttributes);
    test_assert(error != seL4_NoError);
    return SUCCESS;
}
DEFINE_TEST(EPT0008, "Test EPT map and remap PD", test_ept_map_remap_pd)

static int
test_ept_no_overlapping_pt(env_t env, void *args)
{
    int error;
    seL4_CPtr pdpt, pd, pt, frame;
    error = map_ept_set(env, &pdpt, &pd, &pt, &frame);
    test_assert(error == seL4_NoError);

    /* Mapping in a new PT should fail */
    pt = vka_alloc_ept_page_table_leaky(&env->vka);
    test_assert_fatal(pt);
    error = seL4_IA32_EPTPageTable_Map(pt, pdpt, EPT_MAP_BASE, seL4_IA32_Default_VMAttributes);
    test_assert(error != seL4_NoError);
    return SUCCESS;

}
DEFINE_TEST(EPT0009, "Test EPT no overlapping PT", test_ept_no_overlapping_pt)

static int
test_ept_no_overlapping_pd(env_t env, void *args)
{
    int error;
    seL4_CPtr pdpt, pd, pt, frame;
    error = map_ept_set(env, &pdpt, &pd, &pt, &frame);
    test_assert(error == seL4_NoError);

    /* Mapping in a new PT should fail */
    pd = vka_alloc_ept_page_directory_leaky(&env->vka);
    test_assert_fatal(pd);
    error = seL4_IA32_EPTPageDirectory_Map(pd, pdpt, EPT_MAP_BASE, seL4_IA32_Default_VMAttributes);
    test_assert(error != seL4_NoError);
    return SUCCESS;

}
DEFINE_TEST(EPT0010, "Test EPT no overlapping PD", test_ept_no_overlapping_pd)

static int
test_ept_map_remap_pt(env_t env, void *args)
{
    int error;
    seL4_CPtr pdpt, pd, pt, frame;
    error = map_ept_set(env, &pdpt, &pd, &pt, &frame);
    test_assert(error == seL4_NoError);

    /* unmap the pt */
    error = seL4_IA32_EPTPageTable_Unmap(pt);
    test_assert(error == seL4_NoError);

    /* now map it back in */
    error = seL4_IA32_EPTPageTable_Map(pt, pdpt, EPT_MAP_BASE, seL4_IA32_Default_VMAttributes);
    test_assert(error == seL4_NoError);

    /* it should retain its old mappings, and mapping in a new frame should fail */
    frame = vka_alloc_frame_leaky(&env->vka, seL4_PageBits);
    test_assert_fatal(frame);
    error = seL4_IA32_Page_Map(frame, pdpt, EPT_MAP_BASE, seL4_AllRights, seL4_IA32_Default_VMAttributes);
    test_assert(error != seL4_NoError);
    return SUCCESS;
}
DEFINE_TEST(EPT0011, "Test EPT map and remap PT", test_ept_map_remap_pt)

static int
test_ept_recycle_pt(env_t env, void *args)
{
    int error;
    seL4_CPtr pdpt, pd, pt, frame;
    cspacepath_t path;
    error = map_ept_set(env, &pdpt, &pd, &pt, &frame);
    test_assert(error == seL4_NoError);

    /* recycle the pt */
    vka_cspace_make_path(&env->vka, pt, &path);
    error = vka_cnode_recycle(&path);
    test_assert(error == seL4_NoError);

    /* now map a new PT and the same frame */
    pt = vka_alloc_ept_page_table_leaky(&env->vka);
    test_assert_fatal(pt);
    error = seL4_IA32_EPTPageTable_Map(pt, pdpt, EPT_MAP_BASE, seL4_IA32_Default_VMAttributes);
    test_assert(error == seL4_NoError);
    error = seL4_IA32_Page_Map(frame, pdpt, EPT_MAP_BASE, seL4_AllRights, seL4_IA32_Default_VMAttributes);
    test_assert(error == seL4_NoError);

    return SUCCESS;
}
DEFINE_TEST(EPT0012, "Test EPT Recycle PT", test_ept_recycle_pt)

static int
test_ept_recycle_pd(env_t env, void *args)
{
    int error;
    seL4_CPtr pdpt, pd, pt, frame;
    cspacepath_t path;
    error = map_ept_set(env, &pdpt, &pd, &pt, &frame);
    test_assert(error == seL4_NoError);

    /* recycle the pd */
    vka_cspace_make_path(&env->vka, pd, &path);
    error = vka_cnode_recycle(&path);
    test_assert(error == seL4_NoError);

    /* now map a new PD */
    pd = vka_alloc_ept_page_directory_leaky(&env->vka);
    test_assert_fatal(pd);
    error = seL4_IA32_EPTPageDirectory_Map(pd, pdpt, EPT_MAP_BASE, seL4_IA32_Default_VMAttributes);
    test_assert(error == seL4_NoError);

    /* map the old pt back in */
    error = seL4_IA32_EPTPageTable_Map(pt, pdpt, EPT_MAP_BASE, seL4_IA32_Default_VMAttributes);
    test_assert(error == seL4_NoError);

    /* the old frame should still be mapped into the pt, so mapping a new one should fail */
    frame = vka_alloc_frame_leaky(&env->vka, seL4_PageBits);
    test_assert_fatal(frame);
    error = seL4_IA32_Page_Map(frame, pdpt, EPT_MAP_BASE, seL4_AllRights, seL4_IA32_Default_VMAttributes);
    test_assert(error != seL4_NoError);

    return SUCCESS;
}
DEFINE_TEST(EPT0013, "Test EPT recycle PD", test_ept_recycle_pd)

static int
test_ept_recycle_pdpt(env_t env, void *args)
{
    int error;
    seL4_CPtr pdpt, pd, pt, frame;
    cspacepath_t path;
    error = map_ept_set(env, &pdpt, &pd, &pt, &frame);
    test_assert(error == seL4_NoError);

    /* recycle the pdpt */
    vka_cspace_make_path(&env->vka, pdpt, &path);
    error = vka_cnode_recycle(&path);
    test_assert(error == seL4_NoError);

    /* now map the old pd back in */
    error = seL4_IA32_EPTPageDirectory_Map(pd, pdpt, EPT_MAP_BASE, seL4_IA32_Default_VMAttributes);
    test_assert(error == seL4_NoError);

    /* the old pt should still be mapped in */
    pt = vka_alloc_page_table_leaky(&env->vka);
    test_assert_fatal(pt);
    error = seL4_IA32_EPTPageTable_Map(pt, pdpt, EPT_MAP_BASE, seL4_IA32_Default_VMAttributes);
    test_assert(error != seL4_NoError);

    return SUCCESS;
}
DEFINE_TEST(EPT0014, "Test EPT recycle PDPT", test_ept_recycle_pdpt)

#endif /* CONFIG_VTX */
