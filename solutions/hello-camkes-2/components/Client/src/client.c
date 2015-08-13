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

    /* copy strings to an untyped dataport */
    int *n = (int*)d;
    *n = NUM_STRINGS;
    char *str = (char*)(n+1);
    for (int i = 0; i < NUM_STRINGS; i++) {
	strcpy(str, s_arr[i]);
	str += strlen(str) + 1;
    }

    /* emit event to signal that the data is available */
    echo_emit();

    /* wait to get an event back signalling that the reply data is avaialble */
    client_wait();

    /* read the reply data from a typed dataport */
    for (int i = 0; i < d_typed->n; i++) {
	printf("%s: string %d (%p): \"%s\"\n", get_instance_name(), i, d_typed->str[i], d_typed->str[i]);
    }	

    /* send the data over again, this time using two dataports, one typed
     * dataport containing dataport pointers, pointing to data in the 
     * second, untyped, dataport. */
    d_ptrs->n = NUM_STRINGS;
    str = (char*)d;
    for (int i = 0; i < NUM_STRINGS; i++) {
        strcpy(str, s_arr[i]);
	d_ptrs->ptr[i] = dataport_wrap_ptr(str);
        str += strlen(str) + 1;
    }

    /* emit event to signal that the data is available */
    echo_emit();

    /* wait to get an event back signalling that data has been read */
    client_wait();

    /* test the read and write permissions on the dataport.  
     * When we try to write to a read-only dataport, we will get a VM fault. */
    printf("%s: the next instruction will cause a vm fault due to permissions\n", get_instance_name());
    d_typed->n = 0;

    return 0;
}
