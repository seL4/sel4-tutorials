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
#include <ctype.h>

#include <camkes.h>

#include <camkes/dataport.h>

void uppercase(char *str) {
    while (*str != '\0') {
        *str = toupper(*str);
        str++;
    }
}

void echo2_callback_handler(void *a);

void echo_callback_handler(void *a) {
    int *n = (int*)d;
    char *str = (char*)(n+1);
    for (int i = 0, j = *n-1; i < *n; i++, j--) {
    	printf("%s: saying (%p): \"%s\"\n", get_instance_name(), str, str);
	strncpy(d_typed->str[j], str, STR_LEN);
	uppercase(d_typed->str[j]);
	str += strlen(str) + 1;
    }
    d_typed->n = *n;
    echo_reg_callback(echo2_callback_handler, NULL);
    client_emit();
}

void echo2_callback_handler(void *a) {
    char *str;
    for (int i = 0; i < d_ptrs->n; i++) {
	str = dataport_unwrap_ptr(d_ptrs->ptr[i]);
        printf("%s: dptr saying (%p): \"%s\"\n", get_instance_name(), str, str);
    }
    echo_reg_callback(echo_callback_handler, NULL);
    client_emit();
}

void echo__init(void) {
    echo_reg_callback(echo_callback_handler, NULL);
}

