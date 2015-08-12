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

#include <camkes.h>

void hello_say_hello(const char *str) {
    printf("Component %s saying: %s\n", get_instance_name(), str);
}
