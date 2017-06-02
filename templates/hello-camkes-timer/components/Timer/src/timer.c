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

#include <platsupport/mach/epit.h>
#include <platsupport/timer.h>
#include <sel4utils/sel4_zf_logif.h>

#include <camkes.h>

#define NS_IN_SECOND 1000000000ULL

pstimer_t *timer_drv = NULL;

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

    /* TASK: call the platsupport library to handle the interrupt. */
    /* hint: void timer_handle_irq(pstimer_t* device, uint32_t irq)
     * @param device Structure for the timer device driver.
     * @param irq    Timer's interrupt number
     * https://github.com/seL4/util_libs/blob/master/libplatsupport/include/platsupport/timer.h#L159
     */
/*- if solution -*/
    timer_handle_irq(timer_drv, EPIT2_INTERRUPT);
/*- endif -*/

    /* Signal the RPC interface. */
    error = sem_post();
    ZF_LOGF_IF(error != 0, "Failed to post to semaphore");

    /* TASK: acknowledge the interrupt */
    /* hint 1: use the function <IRQ interface name>_acknowledge()
     */
/*- if solution -*/
    error = irq_acknowledge();
    ZF_LOGF_IF(error != 0, "Failed to acknowledge interrupt");
/*- endif -*/
}

void hello__init() {
    /* Structure of the timer configuration in platsupport library */
    epit_config_t config;

    /*
     * Provide hardware info to platsupport.
     */
    config.vaddr = (void*)reg;
    config.irq = EPIT2_INTERRUPT;
    config.prescaler = 0;

    /* TASK: call platsupport library to get the timer handler */
    /* hint: pstimer_t *epit_get_timer(epit_config_t *config);
     * @param config timer configuration structure
     * @return timer handler
     * https://github.com/seL4/util_libs/blob/master/libplatsupport/mach_include/imx/platsupport/mach/epit.h#L28
     */
/*- if solution -*/
    timer_drv = epit_get_timer(&config);
    assert(timer_drv);
/*- endif -*/
}

/* TASK: implement the RPC function. */
/* hint 1: the name of the function to implement is a composition of an interface name and a function name:
 * i.e.: <interface>_<function>
 * hint 2: the interfaces available are defined by the component, e.g. in components/Timer/Timer.camkes
 * hint 3: the function name is defined by the interface definition, e.g. in interfaces/timer.camkes
 * hint 4: so the function would be: hello_sleep()
 * hint 5: the CAmkES 'int' type maps to 'int' in C
 * hint 6: call platsupport library function to set up the timer
 * hint 7: look at https://github.com/seL4/camkes-tool/blob/2.1.0/docs/index.md#creating-an-application
 */
void hello_sleep(int sec) {
    /* TASK: call platsupport library function to set up the timer */
    /* hint: int timer_oneshot_relative(pstimer_t* device, uint64_t ns)
     * @param device timer handler
     * @param ns     timeout in nanoseconds
     * @return 0 on success
     * https://github.com/seL4/util_libs/blob/master/libplatsupport/include/platsupport/timer.h#L146
     */
/*- if solution -*/
    timer_oneshot_relative(timer_drv, sec * NS_IN_SECOND);
/*- endif -*/

    /* Wait for the timeout interrupt */
    int error = sem_wait();
    ZF_LOGF_IF(error != 0, "Failed to wait on semaphore");
}

