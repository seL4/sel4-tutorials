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

void callback_handler_echo_2(void *a);

/* this callback handler is meant to be invoked when the first event 
 * arrives on the echo event interface.
 * Note: the callback handler must be explicitly registered before the 
 * callback will be invoked.  
 * Also the registration is one-shot only, if it wants to be invoked 
 * when a new event arrives then it must re-register itself.  Or it can
 * also register a different handler. */
void callback_handler_echo(void *a) {
    /* TODO: read some data from a dataport */
    
    /* TODO: put a modified copy of it into another dataport */
    
    /* TODO: register the second callback for this event. */

    /* TODO: notify the client that there is new data available for it */
}

/* this callback handler is meant to be invoked the second time an event 
 * arrives on the echo event interface.
 * Note: the callback handler must be explicitly registered before the 
 * callback will be invoked.  
 * Also the registration is one-shot only, if it wants to be invoked 
 * when a new event arrives then it must re-register itself.  Or it can
 * also register a different handler. */
void callback_handler_echo_2(void *a) {
    /* TODO: read some data from the dataports. specifically:
     * read a dataport pointer from one dataport, then use that pointer to 
     * access data in another dataport. */

    /* TODO: register the original callback handler for this event */

    /* TODO: notify the client that we are done reading the data */
}

/* this function is invoked to initialise the echo event interface before it
 * becomes active. */
/* TODO: modify this to reflect the appropriate event interface name */
void echo__init(void) {
    /* TODO: register the first callback handler for this interface */
}

