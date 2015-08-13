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
#include <ctype.h>

#include <camkes/dataport.h>

/* generated header for our component */
#include <camkes.h>

void uppercase(char *str) {
    while (*str != '\0') {
        *str = toupper(*str);
        str++;
    }
}

void callback_handler_echo_2(void *a);

/* this callback handler is meant to be invoked when the first event 
 * arrives on the echo event interface.
 * Note: the callback handler must be explicitly registered before the 
 * callback will be invoked.  
 * Also the registration is one-shot only, if it wants to be invoked 
 * when a new event arrives then it must re-register itself.  Or it can
 * also register a different handler. */
void callback_handler_echo(void *a) {
    /* read some data from a dataport */
    int *n = (int*)d;
    char *str = (char*)(n+1);
    for (int i = 0, j = *n-1; i < *n; i++, j--) {
    	printf("%s: saying (%p): \"%s\"\n", get_instance_name(), str, str);
        /* put a modified copy of it into another dataport */
	strncpy(d_typed->str[j], str, STR_LEN);
	uppercase(d_typed->str[j]);
	str += strlen(str) + 1;
    }
    d_typed->n = *n;
    /* register the second callback for this event. */
    echo_reg_callback(callback_handler_echo_2, NULL);
    /* notify the client that there is new data available for it */
    client_emit();
}

/* this callback handler is meant to be invoked the second time an event 
 * arrives on the echo event interface.
 * Note: the callback handler must be explicitly registered before the 
 * callback will be invoked.  
 * Also the registration is one-shot only, if it wants to be invoked 
 * when a new event arrives then it must re-register itself.  Or it can
 * also register a different handler. */
void callback_handler_echo_2(void *a) {
    /* read some data from the dataports. specifically:
     * read a dataport pointer from one dataport, then use that pointer to 
     * access data in another dataport. */
    char *str;
    for (int i = 0; i < d_ptrs->n; i++) {
	str = dataport_unwrap_ptr(d_ptrs->ptr[i]);
        printf("%s: dptr saying (%p): \"%s\"\n", get_instance_name(), str, str);
    }
    /* register the original callback handler for this event */
    echo_reg_callback(callback_handler_echo, NULL);
    /* notify the client that we are done reading the data */
    client_emit();
}

/* this function is invoked to initialise the echo event interface before it
 * becomes active. */
void echo__init(void) {
    /* register the first callback handler for this interface */
    echo_reg_callback(callback_handler_echo, NULL);
}

