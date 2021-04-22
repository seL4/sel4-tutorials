/*
 * Copyright 2018, Data61, CSIRO (ABN 41 687 119 230).
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

/* Include Kconfig variables. */
#include <autoconf.h>

#include <sel4/sel4.h>
#include <arch_stdio.h>
#include <utils/attribute.h>

/* allow printf to use kernel debug printing */
size_t kernel_putchar_write(void *data, size_t count)
{
#ifdef CONFIG_DEBUG_BUILD
    char *cdata = (char *)data;
    for (size_t i = 0; i < count; i++) {
        seL4_DebugPutChar(cdata[i]);
    }
#endif
    return count;
}

void CONSTRUCTOR(200) register_debug_putchar(void)
{
    sel4muslcsys_register_stdio_write_fn(kernel_putchar_write);
}

/* set a thread's name for debugging purposes */
void name_thread(seL4_CPtr tcb, char *name)
{
#ifdef SEL4_DEBUG_KERNEL
    seL4_DebugNameThread(tcb, name);
#endif
}
