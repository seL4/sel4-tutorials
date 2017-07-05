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

#include <platsupport/timer.h>
#include <sel4platsupport/plat/timer.h>
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

    /* TASK 4: call the platsupport library to handle the interrupt. */
    /* hint: void timer_handle_irq(pstimer_t* device, uint32_t irq)
     * @param device Structure for the timer device driver.
     * @param irq    Timer's interrupt number
     */


    /* TASK 5: Stop the timer. */
    /* hint: void timer_stop(pstimer_t* device)
     * @param device Structure for the timer device driver.
     */


    /* Signal the RPC interface. */
    error = sem_post();
    ZF_LOGF_IF(error != 0, "Failed to post to semaphore");

    /* TASK 6: acknowledge the interrupt */
    /* hint 1: use the function <IRQ interface name>_acknowledge()
     */

}

void hello__init() {
    /* Structure of the timer configuration in platsupport library */
    timer_config_t config;

    /*
     * Provide hardware info to platsupport.
     * Note, The following only works for zynq7000 platform. Other platforms may
     * require other info. Check the definition of timer_config_t and manuals.
     */
    config.vaddr = (void*)reg;
    clk_t clk = clk_generate_fixed_clk(CLK_MASTER, 0);
    config.clk_src = &clk;

    /* TASK 7: call platsupport library to get the timer handler */
    /* hint1: pstimer_t *ps_get_timer(enum timer_id id, timer_config_t *config);
     * hint2: search for TMR_DEFAULT definition to get id
     * @param id the timer to get
     * @param config timer configuration structure
     * @return timer handler
     */

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
    /* hint: int timer_oneshot_absolute(pstimer_t* device, uint64_t ns)
     * @param device timer handler
     * @param ns     timeout in nanoseconds
     * @return 0 on success
     */


    /* TASK 9: Start the timer
     * hint: int timer_start(pstimer_t* device)
     * @param device timer handler
     * @return 0 on success
     */


    int error = sem_wait();
    ZF_LOGF_IF(error != 0, "Failed to wait on semaphore");
}
