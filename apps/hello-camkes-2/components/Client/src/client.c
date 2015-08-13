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
 * CAmkES tutorial part 2: events and dataports
 */

#include <stdio.h>
#include <string.h>

#include <camkes/dataport.h>

/* generated header for our component */
#include <camkes.h>

/* strings to send across to the other component */
char *s_arr[NUM_STRINGS] = { "hello", "world", "how", "are", "you?" };

/* run the control thread */
int run(void) {
    printf("%s: Starting the client\n", get_instance_name());

    /* TODO: copy strings to an untyped dataport */

    /* TODO: emit event to signal that the data is available */

    /* TODO: wait to get event back signalling that reply data is avaialble */

    /* TODO: read the reply data from a typed dataport */

    /* TODO: send the data over again, this time using two dataports, one typed
     * dataport containing dataport pointers, pointing to data in the 
     * second, untyped, dataport. */

    /* TODO: emit event to signal that the data is available */

    /* TODO: wait to get an event back signalling that data has been read */

    /* TODO: test the read and write permissions on the dataport.  
     * When we try to write to a read-only dataport, we will get a VM fault. */

    return 0;
}
