/*
 * Copyright 2018, Data61, CSIRO (ABN 41 687 119 230).
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <sel4/sel4.h>
#include <utils/util.h>

seL4_CPtr alloc_slot(seL4_BootInfo *info)
{
    ZF_LOGF_IF(info->empty.start == info->empty.end, "No CSlots left!");
    seL4_CPtr next_free_slot = info->empty.start++;
    return next_free_slot;
}

/* a very simple allocation function that iterates through the untypeds in boot info until
   a retype succeeds */
seL4_CPtr alloc_object(seL4_BootInfo *info, seL4_Word type)
{
    seL4_CPtr cslot = alloc_slot(info);

    /* keep trying to retype until we succeed */
    seL4_Error error = seL4_NotEnoughMemory;
    for (seL4_CPtr untyped = info->untyped.start; untyped < info->untyped.end; untyped++) {
        seL4_UntypedDesc *desc = &info->untypedList[untyped - info->untyped.start];
        if (!desc->isDevice) {
            seL4_Error error = seL4_Untyped_Retype(untyped, type, 0, seL4_CapInitThreadCNode, 0, 0, cslot, 1);
            if (error == seL4_NoError) {
                return cslot;
            } else if (error != seL4_NotEnoughMemory) {
                ZF_LOGF_IF(error != seL4_NotEnoughMemory, "Failed to allocate untyped");
            }
        }
    }

    ZF_LOGF_IF(error == seL4_NotEnoughMemory, "Out of untyped memory");
    return cslot;
}
