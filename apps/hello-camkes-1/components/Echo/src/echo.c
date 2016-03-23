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
 * CAmkES tutorial part 1: components with RPC. Server part.
 */


#include <stdio.h>

/* generated header for our component */
#include <camkes.h>

/* TODO 5: implement the RPC function. */
/* hint 1: the name of the function to implement is a composition of an interface name and a function name:
 * i.e.: <interface>_<function>
 * hint 2: the interfaces available are defined by the component, e.g. in components/Echo/Echo.camkes
 * hint 3: the function name is defined by the interface definition, e.g. in interfaces/HelloSimple.idl4
 * hint 4: so the function would be: hello_say_hello() 
 * hint 5: the CAmkES 'string' type maps to 'const char *' in C
 * hint 6: make the function print out a mesage using printf
 * hint 7: look at https://github.com/seL4/camkes-tool/blob/2.1.0/docs/index.md#creating-an-application
 */

