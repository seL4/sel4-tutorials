/*
 * Copyright 2018, Data61, CSIRO (ABN 41 687 119 230).
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

/* A very simple, inefficient, non-freeing allocator for doing the tutorials */

/*
 * Allocate a slot from boot info. Allocates slots from info->empty.start.
 * This will not work if slots in the bootinfo empty range have already been used.
 *
 * @return a cslot that has not already been returned by this function, from the range
 *         specified in info->empty
 */
seL4_CPtr alloc_slot(seL4_BootInfo *info);

/*
 * Create an object of the desired type and size.
 *
 * This function iterates through the info->untyped capability range, attempting to
 * retype into an object of the provided type and size, until a successful allocation is made.
 *
 * @param type of the object to create
 * @param size of the object to create. Unused if the object is not variably sized.
 * @return a cslot containing the newly created untyped.
 */
seL4_CPtr alloc_object(seL4_BootInfo *info, seL4_Word type, seL4_Word size_bits);