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

#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include <camkes/dataport.h>
#include <sel4utils/sel4_zf_logif.h>

/* generated header for our component */
#include <camkes.h>

void hello_say_hello(const char *str) {
    printf("Hello %s! From %s\n", get_instance_name(), str);
}