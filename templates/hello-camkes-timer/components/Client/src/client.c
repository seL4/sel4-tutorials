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
 * CAmkES tutorial part 1: components with RPC. Client part.
 */

#include <stdio.h>

/* generated header for our component */
#include <camkes.h>

#define SECS_TO_SLEEP 2

/* run the control thread */
int run(void) {
    printf("Starting the client\n");
    printf("------Sleep for %d seconds------\n", SECS_TO_SLEEP);

    /* invoke the RPC function */
    hello_sleep(SECS_TO_SLEEP);

    printf("After the client: wakeup\n");
    return 0;
}
