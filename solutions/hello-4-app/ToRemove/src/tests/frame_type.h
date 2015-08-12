/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

#ifndef SEL4TEST_TESTS_FRAME_TYPE_H_
#define SEL4TEST_TESTS_FRAME_TYPE_H_

#include <autoconf.h>
#include <stdint.h>
#include <sel4/types.h>
#include <sel4/sel4.h>

static const struct frame_type {
    seL4_Word type;
    seL4_Word vaddr_offset;
    seL4_Word size;
    bool need_pt;
} frame_types[] =
    /* This list must be ordered by size - highest first */
{
#if defined(ARCH_ARM)
#ifdef ARM_HYP
    { seL4_ARM_SuperSectionObject, 0, 1 << 25, false, },
    { seL4_ARM_SectionObject, 1 << 25, 1 << 21, false, },
    { seL4_ARM_LargePageObject, (1 << 25) + (1 << 21), 1 << 16, true, },
    { seL4_ARM_SmallPageObject, (1 << 25) + (1 << 21) + (1 << 16), 1 << 12, true, },
#else
    { seL4_ARM_SuperSectionObject, 0, 1 << 24, false, },
    { seL4_ARM_SectionObject, 1 << 24, 1 << 20, false, },
    { seL4_ARM_LargePageObject, (1 << 24) + (1 << 20), 1 << 16, true, },
    { seL4_ARM_SmallPageObject, (1 << 24) + (1 << 20) + (1 << 16), 1 << 12, true, },
#endif
#elif defined(ARCH_IA32)
#ifdef CONFIG_X86_64
    { seL4_X64_2M, 0, 1 << 21, false, },
    { seL4_X64_4K, 1 << 22, 1 << 12, true, },
#else
    { seL4_IA32_LargePage, 0, 1 << seL4_LargePageBits, false, },
    { seL4_IA32_4K, 1 << 22, 1 << 12, true, },
#endif
#else
#error
#endif
};

#endif /* SEL4TEST_TESTS_FRAME_TYPE_H_ */
