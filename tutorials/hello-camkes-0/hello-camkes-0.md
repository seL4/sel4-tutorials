# CAmkES Tutorial 1
/*? declare_task_ordering(['hello']) ?*/
This tutorial is an introduction to
CAmkES: bootstrapping a basic static CAmkES application, describing its
components, and linking them together.


## Initialising

/*? macros.tutorial_init("hello-camkes-0") ?*/


## Required Reading
 While it's possible to successfully complete the
CAmkES tutorials without having read the
**[CAmkES manuals](https://github.com/seL4/camkes-tool/blob/master/docs/index.md)**, the manuals really explain everything in plain English,
and any aspiring CAmkES dev should read the
**[CAmkES manuals](https://github.com/seL4/camkes-tool/blob/master/docs/index.md)** before attempting to complete the tutorials.

## Learning outcomes


- Understand the structure of a CAmkES application, as a
        described, well-defined, static system.
- Understand the file-layout of a CAmkES ADL project.
- Become acquainted with the basics of creating a practical
        CAmkES application.

## Walkthrough

### TASK 1

The fundamentals of CAmkES are the
component, the interface and the connection. Components are logical
groupings of code and resources. They communicate with other component
instances via well-defined interfaces which must be statically defined,
over communication channels. This tutorial will lead you through the
construction of a CAmkES application with two components: an Echo
server, and its Client that makes calls to it. These components are
defined when you initialise your build repository, found in
the following sub-directories:

- `hello-camkes-1/components/Client/Client.camkes`
- `hello-camkes-1/components/Echo/Echo.camkes`

Find the Component manual section here:
<https://github.com/seL4/camkes-tool/blob/master/docs/index.md#component>

### TASK 2
 The second fundamental component of CAmkES applications
is the Connection: a connection is the representation of a method of
communication between two software components in CAmkES. The underlying
implementation may be shared memory, synchronous IPC, notifications or
some other implementation-provided means. In this particular tutorial,
we are using synchronous IPC. In implementation terms, this boils down
to the seL4_Call() syscall on seL4.

Find the "Connection" keyword manual section here:
<https://github.com/seL4/camkes-tool/blob/master/docs/index.md#connection>

### TASK 3
 All communications over a CAmkES connection must be well
defined: static systems' communications should be able to be reasoned
about at build time. All the function calls which will be delivered over
a communication channel then, also are well defined, and logically
grouped so as to provide clear directional understanding of all
transmissions over a connection. Components are connected together in
CAmkES, yes -- but the interfaces that are exposed over each connection
for calling by other components, are also described. There are different
kinds of interfaces: Dataports, Procedural interfaces and Notifications.

This tutorial will lead you through the construction of a Procedural
interface, which is an interface over which function calls are made
according to a well-defined pre-determined API. The keyword for this
kind of interface in CAmkES is "procedure". The definition of this
Procedure interface may be found here:
`hello-camkes-1/interfaces/HelloSimple.camkes`

Find the "Procedure" keyword definition here:
<https://github.com/seL4/camkes-tool/blob/master/docs/index.md#procedure>

### TASK 4
 Based on the ADL, CAmkES generates boilerplate which
conforms to your system's architecture, and enables you to fill in the
spaces with your program's logic. The two generated files in this
tutorial application are, in accordance with the Components we have
defined:

- `hello-camkes-1/components/Echo/src/echo.c`
- `hello-camkes-1/components/Client/src/client.c`

Now when it comes to invoking the functions that were defined in the
Interface specification
(`hello-camkes-1/interfaces/HelloSimple.camkes`),
you must prefix the API function name with the name of the Interface
instance that you are exposing over the particular connection.

The reason for this is because it is possible for one component to
expose an interface multiple times, with each instance of that interface
referring to a different function altogether. For example, if a
composite device, such as a network card with with a serial interface
integrated into it, exposes two instances of a procedural interface that
has a particular procedure named "send()" -- how will the caller of
"send()" know whether his "send()" is the one that is exposed over the
NIC connection, or the serial connection?

The same component provides both. Therefore, CAmkES prefixes the
instances of functions in an Interface with the Interface-instance's
name. In the dual-function NIC device's case, it might have a
`provides <INTERFACE_NAME> serial` and a `provides <INTERFACE_NAME> nic`.
When a caller wants to call for the NIC-send, it would call,
nic_send(), and when a caller wants to invoke the Serial-send, it would
call, "serial_send()".

So if the "Hello" interface is provided once by "Echo" as "a", you would
call for the "a" instance of Echo's "Hello" by calling for "a_hello()".
But what if Echo had provided 2 instances of the "Hello" interface, and
the second one was named "a2"? Then in order to call on that second
"Hello" interface instance on Echo, you would call "a2_hello()".

Fill in the function calls in the generated C files!

### TASK 5
 Here you define the callee-side invocation functions for
the Hello interface exposed by Echo.

## Done
 Congratulations: be sure to read up on the keywords and
structure of ADL: it's key to understanding CAmkES. And well done on
writing your first CAmkES application.

```c
/*- filter File("hello.camkes") --*/
/* @TAG(DATA61_BSD) */

/*
 * CAmkES tutorial part 0: just a component.
 */

component Client {
    control;
}

assembly {
    composition {
        component Client client;
    }
}

/*-- endfilter -*/
```

```c
/*- filter File("client.c") --*/
/* @TAG(DATA61_BSD) */
/*
 * CAmkES tutorial part 0: just a component.
 */

#include <stdio.h>

/* generated header for our component */
#include <camkes.h>

/* run the control thread */
int run(void) {
    printf("Hello CAmkES World\n");
    return 0;
}
/*-- endfilter -*/
```

```
/*- filter TaskCompletion("hello", TaskContentType.ALL) --*/
Hello CAmkES World
/*-- endfilter -*/
```


```cmake
/*- filter File("CMakeLists.txt") --*/
# @TAG(DATA61_BSD)
cmake_minimum_required(VERSION 3.7.2)

project(hello-camkes-0 C)

ImportCamkes()

DeclareCAmkESComponent(Client SOURCES client.c)

DeclareCAmkESRootserver(hello.camkes)

GenerateCAmkESRootserver()

# utility CMake functions for the tutorials (not required in normal, non-tutorial applications) 
/*? macros.cmake_check_script(state) ?*/
/*-- endfilter -*/
```
