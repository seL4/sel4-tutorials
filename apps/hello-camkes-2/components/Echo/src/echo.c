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

/* hepler function to modify a string to make it all uppercase */
void uppercase(char *str) {
    while (*str != '\0') {
        *str = toupper(*str);
        str++;
    }
}

void callback_handler_2(void *a);

/* this callback handler is meant to be invoked when the first event 
 * arrives on the "consumes" event interface.
 * Note: the callback handler must be explicitly registered before the 
 * callback will be invoked.  
 * Also the registration is one-shot only, if it wants to be invoked 
 * when a new event arrives then it must re-register itself.  Or it can
 * also register a different handler.
 */
void callback_handler_1(void *a) {
    /* TODO 19: read some data from the untyped dataport */
    /* hint 1: use the "Buf" dataport as defined in the Echo.camkes file
     * hint 2: to access the dataport use the interface name as defined in Echo.camkes.
     * For example if you defined it as "dataport Buf d" then you would use "d" to refer to the dataport in C.
     * hint 3: first read the number of strings from the dataport
     * hint 4: then print each string from the dataport
     * hint 5: look at https://github.com/seL4/camkes-tool/blob/2.1.0/docs/index.md#an-example-of-dataports
     */

    /* TODO 20: put a modified copy of the data from the untyped dataport into the typed dataport */
    /* hint 1: modify each string by making it upper case, use the function "uppercase"
     * hint 2: read from the "Buf" dataport as above
     * hint 3: write to the "str_buf_t" dataport as defined in the Echo.camkes file
     * hint 4: to access the dataport use the interface name as defined in Echo.camkes.
     * For example if you defined it as "dataport str_buf_t d_typed" then you would use "d_typed" to refer to the dataport in C.
     * hint 5: for the definition of "str_buf_t" see "str_buf.h":
     *      https://github.com/seL4-projects/sel4-tutorials/blob/master/apps/hello-camkes-2/include/str_buf.h#L24
     * hint 6: use the "n" field to specify the number of strings in the dataport
     * hint 7: copy the specified number of strings from the "Buf" dataport to the "str" field
     * hint 8: look at https://github.com/seL4/camkes-tool/blob/2.1.0/docs/index.md#an-example-of-dataports
     * hint 9: you could combine this TODO with the previous one in a single loop if you want
     */

    /* TODO 21: register the second callback for this event. */
    /* hint 1: use the function <interface name>_reg_callback()
     * hint 2: register the function "callback_handler_2"
     * hint 3: pass NULL as the extra argument to the callback
     * hint 4: look at https://github.com/seL4/camkes-tool/blob/2.1.0/docs/index.md#an-example-of-events
     */

    /* TODO 22: notify the client that there is new data available for it */
    /* hint 1: use the function <interface_name>.emit
     * hint 2: look at https://github.com/seL4/camkes-tool/blob/2.1.0/docs/index.md#an-example-of-events
     */
}

/* this callback handler is meant to be invoked the second time an event 
 * arrives on the "consumes" event interface.
 * Note: the callback handler must be explicitly registered before the 
 * callback will be invoked.  
 * Also the registration is one-shot only, if it wants to be invoked 
 * when a new event arrives then it must re-register itself.  Or it can
 * also register a different handler.
 */
void callback_handler_2(void *a) {
    /* TODO 23: read some data from the dataports. specifically:
     * read a dataport pointer from one of the typed dataports, then use
     * that pointer to access data in the untyped dataport.
     */
    /* hint 1: for the untyped dataport use the "Buf" dataport as defined in the Echo.camkes file
     * hint 2: for the typed dataport use the "ptr_buf_t" dataport as defined in the Echo.camkes file
     * hint 3: for the definition of "ptr_buf_t" see "str_buf.h":
     *      https://github.com/seL4-projects/sel4-tutorials/blob/master/apps/hello-camkes-2/include/str_buf.h#L32
     * hint 4: the "n" field of the typed dataport specifies the number of dataport pointers
     * hint 5: the "ptr" field of the typed dataport contains the dataport pointers
     * hint 6: use the function "dataport_unwrap_ptr()" to create a regular pointer from a dataport pointer
     * hint 7: for more information about dataport pointers see: https://github.com/seL4/camkes-tool/blob/2.1.0/docs/index.md
     * hint 8: print out the string pointed to by each dataport pointer
     */

    /* TODO 24: register the original callback handler for this event */
    /* hint 1: use the function <interface name>_reg_callback()
     * hint 2: register the function "callback_handler_1"
     * hint 3: pass NULL as the extra argument to the callback
     * hint 4: look at https://github.com/seL4/camkes-tool/blob/2.1.0/docs/index.md#an-example-of-events
     */

    /* TODO 25: notify the client that we are done reading the data */
    /* hint 1: use the function <interface_name>.emit
     * hint 2: look at https://github.com/seL4/camkes-tool/blob/2.1.0/docs/index.md#an-example-of-events
     */
}

/* this function is invoked to initialise the echo event interface before it
 * becomes active. */
/* TODO 17: replace "echo" with the actual name of the "consumes" event interface */
/* hint 1: use the interface name as defined in Echo.camkes.
 * For example if you defined it as "consumes TheEvent c_event" then you would use "c_event".
 */
void echo__init(void) {
    /* TODO 18: register the first callback handler for this interface */
    /* hint 1: use the function <interface name>_reg_callback()
     * hint 2: register the function "callback_handler_1"
     * hint 3: pass NULL as the extra argument to the callback
     * hint 4: look at https://github.com/seL4/camkes-tool/blob/2.1.0/docs/index.md#an-example-of-events
     */
}

