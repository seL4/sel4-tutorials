/*
 * Copyright 2020, Data61, CSIRO (ABN 41 687 119 230).
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <utils/util.h>

typedef struct timer_drv {
    uint32_t *reg_base;
    int timer_id;
} timer_drv_t;

int timer_init(timer_drv_t *timer_drv, int timer_id, void *reg_base);
int timer_set_timeout(timer_drv_t *timer_drv, uint64_t ns, bool periodic);
int timer_handle_irq(timer_drv_t *timer_drv);
int timer_start(timer_drv_t *timer_drv);
int timer_stop(timer_drv_t *timer_drv);
