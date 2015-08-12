/*
 * Copyright 2015, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

#include <stdio.h>
#include <string.h>

#include <camkes.h>

#include <camkes/dataport.h>

char *s_arr[NUM_STRINGS] = { "hello", "world", "how", "are", "you?" };

int run(void) {
    printf("%s: Starting the client\n", get_instance_name());

    int *n = (int*)d;
    *n = NUM_STRINGS;
    char *str = (char*)(n+1);
    for (int i = 0; i < NUM_STRINGS; i++) {
	strcpy(str, s_arr[i]);
	str += strlen(str) + 1;
    }

    echo_emit();

    client_wait();

    for (int i = 0; i < d_typed->n; i++) {
	printf("%s: string %d (%p): \"%s\"\n", get_instance_name(), i, d_typed->str[i], d_typed->str[i]);
    }	

    d_ptrs->n = NUM_STRINGS;
    str = (char*)d;
    for (int i = 0; i < NUM_STRINGS; i++) {
        strcpy(str, s_arr[i]);
	d_ptrs->ptr[i] = dataport_wrap_ptr(str);
        str += strlen(str) + 1;
    }

    echo_emit();

    client_wait();

    printf("%s: the next instruction will cause a vm fault due to permissions\n", get_instance_name());
    d_typed->n = 0;

    return 0;
}
