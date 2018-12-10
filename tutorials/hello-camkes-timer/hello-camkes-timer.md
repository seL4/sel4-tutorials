/*#
#
# Copyright 2017, Data61
# Commonwealth Scientific and Industrial Research Organisation (CSIRO)
# ABN 41 687 119 230.
#
# This software may be distributed and modified according to the terms of
# the BSD 2-Clause license. Note that NO WARRANTY is provided.
# See "LICENSE_BSD2.txt" for details.
#
# @TAG(DATA61_BSD)
#
-#*/
# CAmkES Timer Tutorial

This tutorial guides you through setting up a timer driver in CAmkES and using it to delay
for 2 seconds.

## Initialising

/*? macros.tutorial_init("hello-camkes-timer") ?*/

## Prerequisites

1. [Set up your machine](https://docs.sel4.systems/HostDependencies). 
2. [Camkes 2](https://docs.sel4.systems/Tutorials/hello-camkes-2)

## Exercises

### TASK 1


Start in `hello-camkes-timer.camkes`.

Instantiate some components. You're already given one component instance
- `client`. You need to instantiate two more - a timer driver, and a
component instance representing the timer hardware itself. Look in
`components/Timer/Timer.camkes` for the definitions of both of these
components.

Note the lines
`connection seL4RPCCall hello_timer(from client.hello, to timer.hello);`
and `timer.sem_value = 0;`. They assume that the name of the timer
''driver'' will be `timer`. If you wish to call your driver something
else, you'll have to change these lines.

### TASK 2

Connect timer driver to timer hardware. The timer hardware component
exposes two interfaces which must be connected to the timer driver. One
of these represents memory-mapped registers. The other represents an
interrupt.

### TASK 3

Configure the timer hardware component instance with device-specific
info. The physical address of the timer's memory-mapped registers, and
its irq number must both be configured.

### TASK 4

Now open `components/Timer/src/timer.c`.

We'll start by completing the `irq_handle` function, which is called in
response to each timer interrupt. Note the name of this function. It
follows the naming convention `<interface>_handle`, where
`<interface>` is the name of an IRQ interface connected with
`seL4HardwareInterrupt`. When an interrupt is received on the interface
`<interface>`, the function `<interface>_handle` will be
called.

The implementation of the timer driver itself isn't directly in this
file. The driver is implemented in a CAmkES-agnostic way in a library
called `libplatsupport`.

This task is to call the `timer_handle_irq` function from
`libplatsupport`, to inform the driver that an interrupt has occurred.

### TASK 5


Acknowledge the interrupt. CAmkES generates the seL4-specific code for
ack-ing an interrupt and provides a function
`<interface>_acknowldege` for IRQ interfaces (specifically those
connected with `seL4HardwareInterrupt`).

### TASK 6


Now we'll complete `hello__init` - a function which is called once
before the component's interfaces start running.

We need to initialise a timer driver from `libplatsupport` for this
device, and store a handle to the driver in the global variable
`timer_drv`.

### TASK 7


Note that this task is to understand the existing code. You won't have
to modify any files.

Implement the `timer_inf` RPC interface. This interface is defined in
`interfaces/timer.camkes`, and contains a single method, `sleep`, which
should return after a given number of seconds. in
`components/Timer/Timer.camkes`, we can see that the `timer_inf` interface
exposed by the `Timer` component is called `hello`. Thus, the function we
need to implement is called `hello_sleep`.

### TASK 8


Tell the timer to interrupt after the given number of seconds. The
`timer_oneshot_relative` function from `libplatsupport` will help. Note
that it expects its time argument to be given in nanoseconds.

Note the existing code in `hello_sleep`. It waits on a binary semaphore.
`irq_handle` will be called on another thread when the timer interrupt
occurs, and that function will post to the binary semaphore, unblocking
us and allowing the function to return after the delay.


Expect the following output with a 2 second delay between the last 2
lines:
```
Starting the client
------Sleep for 2 seconds------
After the client: wakeup
```

/*? macros.help_block() ?*/

/*-- filter ExcludeDocs() -*/
```c
/*-- filter File("components/Client/Client.camkes") --*/
/* @TAG(DATA61_BSD) */
/*
 * CAmkES tutorial part 1: components with RPC. Client part.
 */

import "../../interfaces/timer.camkes";

component Client {
    control;
    uses timer_inf hello;
}
/*--endfilter -*/
/*-- filter File("components/Client/src/client.c") --*/
/* @TAG(DATA61_BSD) */
/*
 * CAmkES tutorial part 1: components with RPC. Client part.
 */

#include <stdio.h>

/* generated header for our component */
#include <camkes.h>

#define SECS_TO_SLEEP 2

/* run the control thread */
int run(void) {
    printf("Starting the client\n");
    printf("------Sleep for %d seconds------\n", SECS_TO_SLEEP);

    /* invoke the RPC function */
    hello_sleep(SECS_TO_SLEEP);

    printf("After the client: wakeup\n");
    return 0;
}
/*-- endfilter -*/
/*-- filter File("components/Timer/Timer.camkes") --*/
/* @TAG(DATA61_BSD) */
/*
 * Advanced CAmkES tutorial: device driver.
 */
import "../../interfaces/timer.camkes";

component Timerbase{
    hardware;
    dataport Buf reg;
    emits DataAvailable irq;
}

component Timer {
    dataport Buf         reg;
    consumes DataAvailable  irq;

    provides timer_inf     hello;
    has semaphore           sem;
}
/*-- endfilter -*/
/*-- filter File("components/Timer/src/timer.c") --*/
/* @TAG(DATA61_BSD) */
#include <stdio.h>

#include <platsupport/plat/timer.h>
#include <sel4utils/sel4_zf_logif.h>

#include <camkes.h>

#define NS_IN_SECOND 1000000000ULL

ttc_t timer_drv;

/* this callback handler is meant to be invoked when the first interrupt
 * arrives on the interrupt event interface.
 * Note: the callback handler must be explicitly registered before the
 * callback will be invoked.
 * Also the registration is one-shot only, if it wants to be invoked
 * when a new interrupt arrives then it must re-register itself.  Or it can
 * also register a different handler.
 */
void irq_handle(void) {
    int error;

    /* TASK 4: call the platsupport library to handle the interrupt. */
    /* hint: ttc_handle_irq
     */
/*- if solution -*/
    ttc_handle_irq(&timer_drv);
/*- endif -*/

    /* TASK 5: Stop the timer. */
    /* hint: ttc_stop
     */
/*- if solution -*/
    ttc_stop(&timer_drv);
/*- endif -*/

    /* Signal the RPC interface. */
    error = sem_post();
    ZF_LOGF_IF(error != 0, "Failed to post to semaphore");

    /* TASK 6: acknowledge the interrupt */
    /* hint 1: use the function <IRQ interface name>_acknowledge()
     */
/*- if solution -*/
    error = irq_acknowledge();
    ZF_LOGF_IF(error != 0, "Failed to acknowledge interrupt");
/*- endif -*/
}

void hello__init() {
    /* Structure of the timer configuration in platsupport library */
    ttc_config_t config;

    /*
     * Provide hardware info to platsupport.
     * Note, The following only works for zynq7000 platform. Other platforms may
     * require other info. Check the definition of timer_config_t and manuals.
     */
    config.vaddr = (void*)reg;
    config.clk_src = 0;
    config.id = TMR_DEFAULT;

    /* TASK 7: call platsupport library to get the timer handler */
    /* hint1: ttc_init
     */
/*- if solution -*/
    int error = ttc_init(&timer_drv, config);
    assert(error == 0);
/*- endif -*/

    /* TASK 9: Start the timer
     * hint: ttc_start
     */
/*- if solution -*/
    ttc_start(&timer_drv);
/*- endif -*/


}

/* TASK 7: implement the RPC function. */
/* hint 1: the name of the function to implement is a composition of an interface name and a function name:
 * i.e.: <interface>_<function>
 * hint 2: the interfaces available are defined by the component, e.g. in components/Timer/Timer.camkes
 * hint 3: the function name is defined by the interface definition, e.g. in interfaces/timer.camkes
 * hint 4: so the function would be: hello_sleep()
 * hint 5: the CAmkES 'int' type maps to 'int' in C
 * hint 6: call platsupport library function to set up the timer
 * hint 7: look at https://github.com/seL4/camkes-tool/blob/master/docs/index.md#creating-an-application
 */
void hello_sleep(int sec) {
    /* TASK 8: call platsupport library function to set up the timer */
    /* hint: ttc_set_timeout
     */
/*- if solution -*/
    ttc_set_timeout(&timer_drv, sec * NS_IN_SECOND, false);
/*- endif -*/

    int error = sem_wait();
    ZF_LOGF_IF(error != 0, "Failed to wait on semaphore");
}
/*-- endfilter -*/
/*-- filter File("interfaces/timer.camkes") --*/
/* @TAG(DATA61_BSD) */
procedure timer_inf {
    void sleep(in int sec);
};
/*-- endfilter -*/
/*-- filter File("hello-camkes-timer.camkes") --*/
/* @TAG(DATA61_BSD) */
/*
 * CAmkES device driver tutorial.
 */

import <std_connector.camkes>;

/* import the component definitions */
import "components/Client/Client.camkes";
import "components/Timer/Timer.camkes";

/*- if not solution -*/
/* A valid CAmkES assembly must have at least one component. As the starting point for
 * this tutorial does not have any valid components we declare an empty one that does nothing
 * just to construct a valid spec. Once you have added some components to the composition
 * you can remove this if you want, although it will cause no harm being left in */
component EmptyComponent {
}
/*- endif -*/

assembly {
    composition {
/*- if not solution -*/
        component EmptyComponent empty;
/*- endif -*/
        /* TASK 1: component instances */
        /* hint 1: one hardware component, one driver component
         * hint 2: look at
         * https://github.com/seL4/camkes-tool/blob/master/docs/index.md#creating-an-application
         */
/*- if solution -*/
        component Timerbase timerbase;
        component Timer timer;
/*- endif -*/

/*- if solution -*/
        component Client client;
/*- else -*/
        /* this is the component interface. This starts commented out as there it must have
         * its 'hello' interface connected, which initially we cannot do. Uncomment this once
         * you can uncomment the 'timer interface connection' below */
//        component Client client;
/*- endif -*/

        /* TASK 2: connections */
        /* hint 1: use seL4HardwareMMIO to connect device memory
         * hint 2: use seL4HardwareInterrupt to connect interrupt
         * hint 3: look at
         * https://github.com/seL4/camkes-tool/blob/master/docs/index.md#creating-an-application
         */
/*- if solution -*/
        connection seL4HardwareMMIO timer_mem(from timer.reg, to timerbase.reg);
        connection seL4HardwareInterrupt timer_irq(from timerbase.irq, to timer.irq);
/*- endif -*/

        /* timer interface connection */
/*- if solution -*/
        connection seL4RPCCall hello_timer(from client.hello, to timer.hello);
/*- else -*/
        /* uncomment this (and the insantiation of 'client' above) once there is a timer component */
//        connection seL4RPCCall hello_timer(from client.hello, to timer.hello);
/*- endif -*/
    }
    configuration {
        /* TASK 3: hardware resources */
        /* hint 1: find out the device memory address and IRQ number from the hardware data sheet
         * hint 2: look at
         * https://github.com/seL4/camkes-tool/blob/master/docs/index.md#hardware-components
         */
/*- if solution -*/
        timerbase.reg_paddr = 0xF8001000;   // paddr of mmio registers
        timerbase.reg_size = 0x1000;        // size of mmio registers
        timerbase.irq_irq_number = 42;      // timer irq number
/*- endif -*/

        /* assign an initial value to semaphore */
/*- if solution -*/
        timer.sem_value = 0;
/*- else -*/
//        timer.sem_value = 0;
/*- endif -*/
    }
}
/*-- endfilter -*/
/*? ExternalFile("CMakeLists.txt") ?*/
```
/*-- endfilter -*/
