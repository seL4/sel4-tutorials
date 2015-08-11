/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

#include <sel4/sel4.h>
#include <vka/object.h>

#include "../helpers.h"

#include <utils/util.h>

#if CONFIG_HAVE_TIMER

static int
test_interrupt(env_t env, void *args)
{

    int error = timer_periodic(env->timer->timer, 10 * NS_IN_MS);
    timer_start(env->timer->timer);
    sel4_timer_handle_single_irq(env->timer);
    test_check(error == 0);

    for (int i = 0; i < 3; i++) {
        wait_for_timer_interrupt(env);
    }

    timer_stop(env->timer->timer);
    sel4_timer_handle_single_irq(env->timer);

    return SUCCESS;
}
DEFINE_TEST(INTERRUPT0001, "Test interrupts with timer", test_interrupt);
#endif
