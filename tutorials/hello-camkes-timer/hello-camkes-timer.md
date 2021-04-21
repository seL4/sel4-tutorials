<!--
  Copyright 2019, Data61, CSIRO (ABN 41 687 119 230)

  SPDX-License-Identifier: BSD-2-Clause
-->

# CAmkES Timer Tutorial

This tutorial guides you through setting up a sample timer driver component in
CAmkES and using it to delay for 2 seconds. For this tutorial, we will be using
the ZYNQ7000 ARM-based platform. This platform can be simulated via QEMU so it
is not a problem if you do not have access to the actual physical board.

The tutorial also has two parts to it. The first part will teach you how to
manually define hardware details to configure the hardware component and
initialise hardware resources.  The second part will teach you how to use a
CAmkES connector to initialise hardware resources automatically for you.

The solutions to this tutorial primarily uses the method of manually defining
hardware details. The solutions to the second part are also included, albeit
commented out.

## Initialising

/*? macros.tutorial_init("hello-camkes-timer") ?*/

## Prerequisites

1. [Set up your machine](https://docs.sel4.systems/HostDependencies).
2. [Camkes 2](https://docs.sel4.systems/Tutorials/hello-camkes-2)

## Exercises - Part 1

### TASK 1

Start in `hello-camkes-timer.camkes`.

Instantiate some components. You're already given one component instance
- `client`. You need to instantiate additional components, a timer driver and
a component instance representing the timer hardware itself. Look
in `components/Timer/Timer.camkes` for the definitions of the components.

Once you open the file, you will notice three different components. The `Timer`
and `Timerbase` components represents the timer driver and the timer hardware
respectively. The `TimerDTB` component represents both the timer driver and the
timer hardware. This component is meant to be used with the `seL4DTBHardware`
CAmkES connector to automatically initialise hardware resources. The second
part of the tutorial will go into more detail about the `TimerDTB` component
and the `seL4DTBHardware` connector.

For now, instantiate the `Timer` and `Timerbase` components.

Note the lines `connection seL4RPCCall hello_timer(from client.hello, to
timer.hello);` and `timer.sem_value = 0;` in the `hello-camkes-timer.camkes`
file. They assume that the name of the timer ''driver'' will be `timer`. If you
wish to call your driver something else, you'll have to change these lines.

### TASK 2

Connect the timer driver component (`Timer`) to the timer hardware component
(`Timerbase`). The timer hardware component exposes two interfaces which must
be connected to the timer driver. One of these represents memory-mapped
registers. The other represents an interrupt.

### TASK 3

Configure the timer hardware component instance with device-specific info. The
physical address of the timer's memory-mapped registers, and its IRQ number
must both be configured.

### TASK 4

Now open `components/Timer/src/timer.c`.

We'll start by completing the `irq_handle` function, which is called in
response to each timer interrupt. Note the name of this function. It follows
the naming convention `<interface>_handle`, where `<interface>` is the name of
an IRQ interface connected with `seL4HardwareInterrupt`. When an interrupt is
received on the interface `<interface>`, the function `<interface>_handle` will
be called.

The implementation of the timer driver is located inside a different folder and
can be found in the `projects/sel4-tutorials/zynq_timer_driver` folder from the
root of the projects directory, i.e. where the `.repo` folder can be found and
where the initial `repo init` command was executed.

This task is to call the `timer_handle_irq` function from the supply driver to
inform the driver that an interrupt has occurred.

### TASK 5

Stop the timer from running. The `timer_stop` function will be helpful here.

### TASK 6

The interrupt now needs to be acknowledged.

CAmkES generates the seL4-specific code for ack-ing an interrupt and provides a
function `<interface>_acknowldege` for IRQ interfaces (specifically those
connected with `seL4HardwareInterrupt`).

### TASK 7

Now we'll complete `hello__init` - a function which is called once
before the component's interfaces start running.

We need to initialise a handle to the timer driver for this device, and store a
handle to the driver in the global variable `timer_drv`.

### TASK 8

After initialising the timer, we now need to start the timer. Do so by calling
`timer_start` and passing the handle to the driver.

### TASK 9

Note that this task is to understand the existing code. You won't have
to modify anything for this task.

Implement the `timer_inf` RPC interface. This interface is defined in
`interfaces/timer.camkes`, and contains a single method, `sleep`, which
should return after a given number of seconds. in
`components/Timer/Timer.camkes`, we can see that the `timer_inf` interface
exposed by the `Timer` component is called `hello`. Thus, the function we
need to implement is called `hello_sleep`.

### TASK 10

Tell the timer to interrupt after the given number of seconds. The
`timer_set_timeout` function from the included driver will help. Note that it
expects its time argument to be given in nanoseconds.

Note the existing code in `hello_sleep`. It waits on a binary semaphore.
`irq_handle` will be called on another thread when the timer interrupt occurs,
and that function will post to the binary semaphore, unblocking us and allowing
the function to return after the delay.

Expect the following output with a 2 second delay between the last 2
lines:
```
Starting the client
------Sleep for 2 seconds------
After the client: wakeup
```

## Exercises - Part 2

Now that you've learnt how to manually define the hardware details of a
hardware component to initialise hardware resources, this part of the tutorial
will teach you how to use the `seL4DTBHardware` connector to do that
automatically.

The connector requires a devicetree blob which describes an ARM platform.
Additionally, the blob's interrupt fields also need to follow the same format
of the ARM GIC v1 and v2. There are devicetree source files bundled with the
kernel, look in the `tools/dts/` folder of the kernel sources. If a suitable
devicetree blob is not available for your platform, then do not proceed with
the tutorial.

### TASK 1

Navigate to the `hello-camkes-timer.camkes` file.

Remove the `Timerbase` and `Timer` component instantiations and instantiate a
`TimerDTB` component instead. Also change the `connection seL4RPCCall
hello_timer(from client.hello, to timer.hello);` and `timer.sem_value = 0;`
lines if necessary.

### TASK 2

Remove the `seL4HardwareMMIO` and `seL4HardwareInterrupt` connections. Connect
the two interfaces inside the `TimerDTB` component with the `seL4DTBHardware`
connector.

### TASK 3

Before opening `components/Timer/Timer.camkes`, remove the `Timerbase` settings
inside the configurations block.

Configure the `TimerDTB` component to pass in the correct DTB path to a timer
to the connector and also initialise the interrupt resources for the timer.
This will allow the connector to read a device node from the devicetree blob
and grab the necessary data to initialise hardware resources. More
specifically, it reads the registers field and optionally the interrupts field
to allocate memory and interrupts.

### TASK 4

Move to `components/TimerDTB/src/timerdtb.c`.

Similar to part one, we'll start with the `tmr_irq_handle` function. This
function is called in response to a timer interrupt. The name of the function
has a special meaning and the meaning is unique to the `seL4DTBHardware`
connector.

The IRQ handling functions of the connector follows the naming convention
`<to_interface>_irq_handle`, where `<to_interface>` is the name of the
interface of the 'to' end in an instance of a `seL4DTBHardware` connection.
Also notice that it takes a `ps_irq_t *` type. This is because, for a given
device, there may be multiple interrupts associated with the device. A
`ps_irq_t` struct is given to the IRQ handling function and it contains
information about the interrupt, allowing the handler to differentiate between
the numerous interrupts of a device.

Likewise with part one, the implementation of the timer driver is in the
included driver in `timer_driver` and the task here is to call
`timer_handle_irq`.

### TASK 5

The timer needs to be stopped, the task here is the same as part one's task 5.

### TASK 6

Again, the interrupt now has to be acknowledged.

For the `seL4DTBHardware` connector, CAmkES also generates and provides a
function to acknowledge interrupts. This function follows a similar naming
convention to the IRQ handler above, `<to_interface>_irq_acknowledge` also
takes in a `ps_irq_t *` argument. Similarly, the `ps_irq_t *` argument helps
CAmkES to differentiate between the possibly many interrupts of a device that
you wish to acknowledge.

### TASK 7 - 10

Task 7 to 10 are the exact same as the tasks in part one.

You should also expect the same output as the first part.

/*? macros.help_block() ?*/

/*-- filter ExcludeDocs() -*/
```c
/*-- filter File("components/Client/Client.camkes") --*/
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

component TimerDTB {
    emits Dummy dummy_source;
    consumes Dummy tmr;

    provides timer_inf hello;
    has semaphore sem;

/* This is intentionally empty. Setting configurations inside a component requires a composition
 * block, just limitations of CAmkES. */
    composition {
    }

    configuration {
    /* Part 2, TASK 3: hardware resources */
    /* TimerDTB:
     * hint 1: look in the DTB/DTS for the path of a timer
     * hint 2: set the 'dtb' setting for the tmr interface in the TimerDTB component,
     *         e.g. foo.dtb = dtb({"path" : "/bar"});
     * hint 3: set the 'generate_interrupts' setting to 1
     */
/*- if solution -*/
//        tmr.dtb = dtb({"path" : "/amba/timer@f8001000"});   // path of the timer in the DTB
//        tmr.generate_interrupts = 1;                        // tell seL4DTBHardware to init interrupts
/*- endif -*/
    }
}
/*-- endfilter -*/

/* ExternalFile("timer_driver/src/driver.c") */
/* ExternalFile("timer_driver/include/timer_driver/driver.h") */

/*-- filter File("components/Timer/src/timer.c") --*/
#include <stdio.h>

#include <timer_driver/driver.h>

#include <camkes.h>

#define NS_IN_SECOND 1000000000ull
#define DEFAULT_TIMER_ID 0

timer_drv_t timer_drv;

void irq_handle(void) {
    int error;

    /* Part 1, TASK 4: call into the supplied driver to handle the interrupt. */
    /* hint: timer_handle_irq
     */
/*- if solution -*/
    timer_handle_irq(&timer_drv);
/*- endif -*/

    /* Part 1, TASK 5: stop the timer. */
    /* hint: timer_stop
     */
/*- if solution -*/
    timer_stop(&timer_drv);
/*- endif -*/

    /* signal the rpc interface. */
    error = sem_post();
    ZF_LOGF_IF(error != 0, "failed to post to semaphore");

    /* Part 1, TASK 6: acknowledge the interrupt */
    /* hint 1: use the function <irq interface name>_acknowledge()
     */
/*- if solution -*/
    error = irq_acknowledge();
    ZF_LOGF_IF(error != 0, "failed to acknowledge interrupt");
/*- endif -*/
}

void hello__init() {
    /* Part 1, TASK 7: call into the supplied driver to get the timer handler */
    /* hint1: timer_init
     * hint2: The timer ID is supplied as a #define in this file
     * hint3: The register's variable name is the same name as the dataport in the Timer component
     */
/*- if solution -*/
    int error = timer_init(&timer_drv, DEFAULT_TIMER_ID, reg);
    assert(error == 0);
/*- endif -*/

    /* Part 1, TASK 8: start the timer
     * hint: timer_start
     */
/*- if solution -*/
    error = timer_start(&timer_drv);
    assert(error == 0);
/*- endif -*/
}

/* part 1, TASK 9: implement the rpc function. */
/* hint 1: the name of the function to implement is a composition of an interface name and a function name:
 * i.e.: <interface>_<function>
 * hint 2: the interfaces available are defined by the component, e.g. in components/timer/timer.camkes
 * hint 3: the function name is defined by the interface definition, e.g. in interfaces/timer.camkes
 * hint 4: so the function would be: hello_sleep()
 * hint 5: the camkes 'int' type maps to 'int' in c
 * hint 6: invoke a function in supplied driver the to set up the timer
 * hint 7: look at https://github.com/sel4/camkes-tool/blob/master/docs/index.md#creating-an-application
 */
void hello_sleep(int sec) {
    int error = 0;

    /* Part 1, TASK 10: invoke a function in the supplied driver to set a timeout */
    /* hint1: timer_set_timeout
     * hint2: periodic should be set to false
     */
/*- if solution -*/
    error = timer_set_timeout(&timer_drv, sec * NS_IN_SECOND, false);
    assert(error == 0);
/*- endif -*/

    error = sem_wait();
    ZF_LOGF_IF(error != 0, "failed to wait on semaphore");
}
/*-- endfilter -*/

/*-- filter File("components/TimerDTB/src/timerdtb.c") --*/
#include <stdio.h>

#include <timer_driver/driver.h>

#include <camkes.h>

#define NS_IN_SECOND 1000000000ull
#define DEFAULT_TIMER_ID 0

timer_drv_t timer_drv;

void tmr_irq_handle(ps_irq_t *irq) {
    int error;

    /* Part 2, TASK 4: call into the supplied driver to handle the interrupt. */
    /* hint: timer_handle_irq
     */
/*- if solution -*/
    timer_handle_irq(&timer_drv);
/*- endif -*/

    /* Part 2, TASK 5: stop the timer. */
    /* hint: timer_stop
     */
/*- if solution -*/
    timer_stop(&timer_drv);
/*- endif -*/

    /* signal the rpc interface. */
    error = sem_post();
    ZF_LOGF_IF(error != 0, "failed to post to semaphore");

    /* Part 2, TASK 6: acknowledge the interrupt */
    /* hint 1: use the function <to_interface_name>_irq_acknowledge()
     * hint 2: pass in the 'irq' variable to the function
     */
/*- if solution -*/
    error = tmr_irq_acknowledge(irq);
    ZF_LOGF_IF(error != 0, "failed to acknowledge interrupt");
/*- endif -*/
}

void hello__init() {
    /* Part 2, TASK 7: call into the supplied driver to get the timer handler */
    /* hint1: timer_init
     * hint2: The timer ID is supplied as a #define in this file
     * hint3: The register's name is follows the format of <interface name>_<register number>,
     * where "interface name" is the name of the
     * interface where you set the path of the DTB (foo.dtb = dtb({...}))
     * and "register number" is the index of the register block in the device
     * node in the devicetree blob
     */
/*- if solution -*/
    int error = timer_init(&timer_drv, DEFAULT_TIMER_ID, tmr_0);
    assert(error == 0);
/*- endif -*/

    /* Part 2, TASK 8: start the timer
     * hint: timer_start
     */
/*- if solution -*/
    timer_start(&timer_drv);
/*- endif -*/
}

/* Part 2, TASK 9: implement the rpc function. */
/* hint 1: the name of the function to implement is a composition of an interface name and a function name:
 * i.e.: <interface>_<function>
 * hint 2: the interfaces available are defined by the component, e.g. in components/timer/timer.camkes
 * hint 3: the function name is defined by the interface definition, e.g. in interfaces/timer.camkes
 * hint 4: so the function would be: hello_sleep()
 * hint 5: the camkes 'int' type maps to 'int' in c
 * hint 6: invoke a function in the supplied driver to set a timeout
 * hint 7: look at https://github.com/sel4/camkes-tool/blob/master/docs/index.md#creating-an-application
 */
void hello_sleep(int sec) {
    int error = 0;

    /* Part 2, TASK 10: invoke a function in the supplied driver to set a timeout */
    /* hint1: timer_set_timeout
     * hint2: periodic should be set to false
     */
/*- if solution -*/
    error = timer_set_timeout(&timer_drv, sec * NS_IN_SECOND, false);
    assert(error == 0);
/*- endif -*/

    error = sem_wait();
    ZF_LOGF_IF(error != 0, "failed to wait on semaphore");
}
/*-- endfilter -*/
/*-- filter File("interfaces/timer.camkes") --*/
procedure timer_inf {
    void sleep(in int sec);
};
/*-- endfilter -*/
/*-- filter File("hello-camkes-timer.camkes") --*/
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
        /* Part 1, TASK 1: component instances */
        /* hint 1: one hardware component and one driver component
         * hint 2: look at
         * https://github.com/seL4/camkes-tool/blob/master/docs/index.md#creating-an-application
         */

        /* Part 2, TASK 1: component instances */
        /* hint 1: a single TimerDTB component
         * hint 2: look at
         * https://github.com/seL4/camkes-tool/blob/master/docs/index.md#creating-an-application
         */
/*- if solution -*/
        component Timerbase timerbase;
        component Timer timer;

//        component TimerDTB timer;
/*- endif -*/

/*- if solution -*/
        component Client client;
/*- else -*/
        /* this is the component interface. This starts commented out as there it must have
         * its 'hello' interface connected, which initially we cannot do. Uncomment this once
         * you can uncomment the 'timer interface connection' below */
//        component Client client;
/*- endif -*/

        /* Part 1, TASK 2: connections */
        /* hint 1: use seL4HardwareMMIO to connect device memory
         * hint 2: use seL4HardwareInterrupt to connect interrupt
         * hint 3: look at
         * https://github.com/seL4/camkes-tool/blob/master/docs/index.md#creating-an-application
         */

        /* Part 2, TASK 2: connections */
        /* hint 1: connect the dummy_source and timer interfaces
         * hint 2: the dummy_source should be the 'from' end
         * hint 3: look at
         * https://github.com/seL4/camkes-tool/blob/master/docs/index.md#creating-an-application
         */
/*- if solution -*/
        connection seL4HardwareMMIO timer_mem(from timer.reg, to timerbase.reg);
        connection seL4HardwareInterrupt timer_irq(from timerbase.irq, to timer.irq);

//        connection seL4DTBHardware timer_dtb(from timer.dummy_source, to timer.tmr);
/*- endif -*/

        /* timer interface connection */
/*- if solution -*/
        connection seL4RPCCall hello_timer(from client.hello, to timer.hello);
/*- else -*/
        /* uncomment this (and the insantiation of 'client' above) once there is a timer component,
         * note that you may need to rename timer in 'to timer.hello' to match the name of the timer
         * component above
         */
//        connection seL4RPCCall hello_timer(from client.hello, to timer.hello);
/*- endif -*/
    }
    configuration {
        /* Part 1, TASK 3: hardware resources */
        /* Timer and Timerbase:
         * hint 1: find out the device memory address and IRQ number from the hardware data sheet
         * hint 2: look at
         * https://github.com/seL4/camkes-tool/blob/master/docs/index.md#hardware-components
         */

        /* Part 2, TASK 3: hardware resources */
        /* TimerDTB:
         * check components/Timer/Timer.camkes
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
