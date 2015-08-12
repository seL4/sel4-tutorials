/*
 * Copyright 2015, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

/* Include Kconfig variables. */
#include <autoconf.h>

#include <stdio.h>

#include <sel4/sel4.h>

void abort(void) {
    while (1);
}

void __arch_putchar(int c) {
#ifdef CONFIG_DEBUG_BUILD
    seL4_DebugPutChar(c);
#endif
}


int main(void)
{
    printf("hello world\n");

    return 0;
}

