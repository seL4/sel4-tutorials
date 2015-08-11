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

#ifdef CONFIG_KERNEL_STABLE

static int
test_retype(env_t env, void* args)
{
    /* TODO: Write some tests\n */

    printf("Warning: Untype tests on stable have not been written\n");

    return SUCCESS;
}
DEFINE_TEST(RETYPE0000, "Retype at offset [dummy] test", test_retype)

#endif /* CONFIG_KERNEL_STABLE */
