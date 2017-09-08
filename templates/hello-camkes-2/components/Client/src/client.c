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

    /* TASK 9: copy strings to an untyped dataport */
    /* hint 1: use the "Buf" dataport as defined in the Client.camkes file
     * hint 2: to access the dataport use the interface name as defined in Client.camkes.
     * For example if you defined it as "dataport Buf d" then you would use "d" to refer to the dataport in C.
     * hint 3: first write the number of strings (NUM_STRINGS) to the dataport
     * hint 4: then copy all the strings from "s_arr" to the dataport.
     * hint 5: look at https://github.com/seL4/camkes-tool/blob/master/docs/index.md#an-example-of-dataports
     */
/*- if solution -*/
    int *n = (int*)d;
    *n = NUM_STRINGS;
    char *str = (char*)(n + 1);
    for (int i = 0; i < NUM_STRINGS; i++) {
        strcpy(str, s_arr[i]);
        str += strlen(str) + 1;
    }
/*- endif -*/

    /* TASK 10: emit event to signal that the data is available */
    /* hint 1: use the function <interface_name>_emit
     * hint 2: look at https://github.com/seL4/camkes-tool/blob/master/docs/index.md#an-example-of-events
     */
/*- if solution -*/
    echo_emit();
/*- endif -*/

    /* TASK 11: wait to get an event back signalling that the reply data is avaialble */
    /* hint 1: use the function <interface_name>.wait
     * hint 2: look at https://github.com/seL4/camkes-tool/blob/master/docs/index.md#an-example-of-events
     */
/*- if solution -*/
    client_wait();
/*- endif -*/

    /* TASK 12: read the reply data from a typed dataport */
    /* hint 1: use the "str_buf_t" dataport as defined in the Client.camkes file
     * hint 2: to access the dataport use the interface name as defined in Client.camkes.
     * For example if you defined it as "dataport str_buf_t d_typed" then you would use "d_typed" to refer to the dataport in C.
     * hint 3: for the definition of "str_buf_t" see "str_buf.h".
     * hint 4: use the "n" field to determine the number of strings in the dataport
     * hint 5: print out the specified number of strings from the "str" field
     * hint 6: look at https://github.com/seL4/camkes-tool/blob/master/docs/index.md#an-example-of-dataports
     */
/*- if solution -*/
    for (int i = 0; i < d_typed->n; i++) {
        printf("%s: string %d (%p): \"%s\"\n", get_instance_name(), i, d_typed->str[i], d_typed->str[i]);
    }
/*- endif -*/

    /* TASK 13: send the data over again, this time using two dataports, one
     * untyped dataport containing the data, and one typed dataport containing
     * dataport pointers pointing to data in the untyped, dataport.
     */
    /* hint 1: for the untyped dataport use the "Buf" dataport as defined in the Client.camkes file
     * hint 2: for the typed dataport use the "ptr_buf_t" dataport as defined in the Client.camkes file
     * hint 3: for the definition of "ptr_buf_t" see "str_buf.h".
     * hint 4: copy all the strings from "s_arr" into the untyped dataport
     * hint 5: use the "n" field of the typed dataport to specify the number of dataport pointers (NUM_STRINGS)
     * hint 6: use the "ptr" field of the typed dataport to store the dataport pointers
     * hint 7: use the function "dataport_wrap_ptr()" to create a dataport pointer from a regular pointer
     * hint 8: the dataport pointers should point into the untyped dataport
     * hint 9: for more information about dataport pointers see: https://github.com/seL4/camkes-tool/blob/master/docs/index.md
     */
/*- if solution -*/
    d_ptrs->n = NUM_STRINGS;
    str = (char*)d;
    for (int i = 0; i < NUM_STRINGS; i++) {
        strcpy(str, s_arr[i]);
        d_ptrs->ptr[i] = dataport_wrap_ptr(str);
        str += strlen(str) + 1;
    }
/*- endif -*/

    /* TASK 14: emit event to signal that the data is available */
    /* hint 1: we've already done this before */
/*- if solution -*/
    echo_emit();
/*- endif -*/

    /* TASK 15: wait to get an event back signalling that data has been read */
    /* hint 1: we've already done this before */
/*- if solution -*/
    client_wait();
/*- endif -*/

    printf("%s: the next instruction will cause a vm fault due to permissions\n", get_instance_name());

    /* TASK 16: test the read and write permissions on the dataport.
     * When we try to write to a read-only dataport, we will get a VM fault.
     */
    /* hint 1: try to assign a value to a field of the "str_buf_t" dataport */
/*- if solution -*/
    d_typed->n = 0;
/*- endif -*/

    return 0;
}
