/*
 * Copyright 2015, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

/*
 * helper functions
 */

/* Include Kconfig variables. */
#include <autoconf.h>

#include <sel4/sel4.h>

/* avoid main falling off the end of the world */
void abort(void) {
    while (1);
}

/* allow printf to use kernel debug printing */
void __arch_putchar(int c) {
#ifdef CONFIG_DEBUG_BUILD
    seL4_DebugPutChar(c);
#endif
}

/* set a thread's name for debugging purposes */
void name_thread(seL4_CPtr tcb, char *name) {
#ifdef SEL4_DEBUG_KERNEL
    seL4_DebugNameThread(tcb, name);
#endif
}



