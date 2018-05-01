/*
 * Copyright 2017, Data61
 * Commonwealth Scientific and Industrial Research Organisation (CSIRO)
 * ABN 41 687 119 230.
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(DATA61_BSD)
 */

/* Include Kconfig variables. */
#include <autoconf.h>

#include <sel4/sel4.h>
#include <arch_stdio.h>
#include <utils/attribute.h>

/* allow printf to use kernel debug printing */
size_t kernel_putchar_write(void *data, size_t count) {
#ifdef CONFIG_DEBUG_BUILD
    char *cdata = (char*)data;
    for (size_t i = 0; i < count; i++) {
        seL4_DebugPutChar(cdata[i]);
    }
#endif
    return count;
}

void CONSTRUCTOR(200) register_debug_putchar(void) {
    sel4muslcsys_register_stdio_write_fn(kernel_putchar_write);
}

/* set a thread's name for debugging purposes */
void name_thread(seL4_CPtr tcb, char *name) {
#ifdef SEL4_DEBUG_KERNEL
    seL4_DebugNameThread(tcb, name);
#endif
}
