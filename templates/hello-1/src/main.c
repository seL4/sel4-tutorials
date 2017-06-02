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

/*
 * seL4 tutorial part 1:  simple printf
 */


#include <stdio.h>

#include <utils/zf_log.h>
#include <sel4utils/sel4_zf_logif.h>

/* TASK 1: add a main function to print a message */
/* hint: printf */

/*- if solution -*/
int main(void) {
    printf("hello world\n");

    return 0;
}
/*- endif -*/
