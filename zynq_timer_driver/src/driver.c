/*
 * Copyright 2020, Data61, CSIRO (ABN 41 687 119 230).
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

/* Basic driver for the TTC timers on the ZYNQ7000 */

#include <timer_driver/driver.h>

#define CLK_CTRL_OFFSET 0x00
#define CNT_CTRL_OFFSET 0x0C
#define CNT_VAL_OFFSET 0x18
#define INTERVAL_OFFSET 0x24
#define MATCH0_OFFSET 0x30
#define INT_STS_OFFSET 0x54
#define INT_EN_OFFSET 0x60

#define CLK_CTRL_PRESCALE_VAL(N) (((N) & 0xf) << 1)
#define CLK_CTRL_PRESCALE_ENABLE BIT(0)
#define CLK_CTRL_PRESCALE_MASK (CLK_CTRL_PRESCALE_VAL(0xf) | CLK_CTRL_PRESCALE_ENABLE)

#define CNT_CTRL_STOP BIT(0) /* Stop the counter */
#define CNT_CTRL_INT BIT(1) /* Set timer to interval mode */
#define CNT_CTRL_MATCH BIT(3) /* Generate interrupt when counter matches value in match register */
#define CNT_CTRL_RST BIT(4) /* Reset counter value and restart counting */

#define INT_EN_INTERVAL BIT(0) /* Enable interval interrupts */
#define INT_EN_MATCH0 BIT(1) /* Enable match 1 interrupt */

#define INTERVAL_CNT_WIDTH 16
#define INTERVAL_CNT_MAX (BIT(INTERVAL_CNT_WIDTH) - 1)

#define PCLK_FREQ 111000000UL
#define PRESCALE_MAX 0xf

static inline uint32_t timer_get_register(timer_drv_t *timer_drv, size_t offset)
{
    assert(timer_drv);
    volatile uint32_t *target_reg = (void *) timer_drv->reg_base + offset + (timer_drv->timer_id * 4);
    return (uint32_t)(*target_reg);
}

static inline void timer_set_register(timer_drv_t *timer_drv, size_t offset, uint32_t value)
{
    assert(timer_drv);
    volatile uint32_t *target_reg = (void *) timer_drv->reg_base + offset + (timer_drv->timer_id * 4);
    *target_reg = value;
}

static int timer_set_freq_for_ns(timer_drv_t *timer_drv, uint64_t ns, uint64_t *ret_interval)
{
    assert(ret_interval);

    uint64_t required_freq = freq_cycles_and_ns_to_hz(INTERVAL_CNT_MAX, ns);

    /* Find a prescale value that gets us close to the required frequency */
    int prescale = 0;
    uint64_t curr_freq = PCLK_FREQ;
    for (prescale = 0; curr_freq > required_freq; prescale++, curr_freq >>= 1);
    if (prescale > PRESCALE_MAX) {
        /* Couldn't find a suitable value for the prescale */
        return -1;
    }

    /* Set the prescale */
    uint32_t to_set = timer_get_register(timer_drv, CLK_CTRL_OFFSET) & ~CLK_CTRL_PRESCALE_MASK;
    if (prescale > 0) {
        to_set |= CLK_CTRL_PRESCALE_ENABLE | CLK_CTRL_PRESCALE_VAL(prescale - 1);
    } else {
        to_set &= ~CLK_CTRL_PRESCALE_ENABLE;
    }
    timer_set_register(timer_drv, CLK_CTRL_OFFSET, to_set);

    *ret_interval = freq_ns_and_hz_to_cycles(ns, curr_freq);
    assert(*ret_interval <= INTERVAL_CNT_MAX);

    return 0;
}

static void timer_periodic_timeout(timer_drv_t *timer_drv, uint64_t counter_val)
{
    timer_set_register(timer_drv, INTERVAL_OFFSET, counter_val);
    uint32_t cnt_ctrl = timer_get_register(timer_drv, CNT_CTRL_OFFSET);
    cnt_ctrl |= CNT_CTRL_INT;
    timer_set_register(timer_drv, CNT_CTRL_OFFSET, cnt_ctrl);
    timer_set_register(timer_drv, INT_EN_OFFSET, INT_EN_INTERVAL);
}

static void timer_oneshot_timeout(timer_drv_t *timer_drv, uint64_t counter_val)
{
    /* Set the match register to the interval that we calculated */
    uint32_t match_to_set = (counter_val + timer_get_register(timer_drv, CNT_VAL_OFFSET)) % BIT(INTERVAL_CNT_WIDTH);
    timer_set_register(timer_drv, MATCH0_OFFSET, match_to_set);

    /* Turn off interval interrupts and turn on the match interrupt */
    uint32_t cnt_ctrl = timer_get_register(timer_drv, CNT_CTRL_OFFSET);
    cnt_ctrl &= ~CNT_CTRL_INT;
    timer_set_register(timer_drv, CNT_CTRL_OFFSET, cnt_ctrl);
    timer_set_register(timer_drv, INT_EN_OFFSET, INT_EN_MATCH0);
}

int timer_set_timeout(timer_drv_t *timer_drv, uint64_t ns, bool periodic)
{
    assert(timer_drv);
    uint64_t counter_interval = 0;

    int error = timer_set_freq_for_ns(timer_drv, ns, &counter_interval);
    if (error) {
        return -1;
    }

    if (periodic) {
        timer_periodic_timeout(timer_drv, counter_interval);
    } else {
        timer_oneshot_timeout(timer_drv, counter_interval);
    }

    return 0;
}

int timer_handle_irq(timer_drv_t *timer_drv)
{
    assert(timer_drv);
    /* Read the interrupt status register to clear the interrupt bit */
    FORCE_READ(timer_drv->reg_base + (INT_STS_OFFSET / sizeof(uint32_t)));

    uint32_t int_en = timer_get_register(timer_drv, INT_EN_OFFSET);
    int_en &= ~INT_EN_MATCH0;
    timer_set_register(timer_drv, INT_EN_OFFSET, int_en);

    return 0;
}

int timer_start(timer_drv_t *timer_drv)
{
    assert(timer_drv);
    uint32_t cnt_ctrl = timer_get_register(timer_drv, CNT_CTRL_OFFSET);
    cnt_ctrl &= ~CNT_CTRL_STOP;
    timer_set_register(timer_drv, CNT_CTRL_OFFSET, cnt_ctrl);
    return 0;
}

int timer_stop(timer_drv_t *timer_drv)
{
    assert(timer_drv);
    uint32_t cnt_ctrl = timer_get_register(timer_drv, CNT_CTRL_OFFSET);
    cnt_ctrl |= CNT_CTRL_STOP;
    timer_set_register(timer_drv, CNT_CTRL_OFFSET, cnt_ctrl);
    return 0;
}

int timer_init(timer_drv_t *timer_drv, int timer_id, void *reg_base)
{
    assert(timer_drv);
    assert(reg_base);
    assert(0 <= timer_id && timer_id <= 2);

    timer_drv->reg_base = reg_base;
    timer_drv->timer_id = timer_id;

    timer_set_register(timer_drv, INT_EN_OFFSET, 0);
    FORCE_READ(timer_drv->reg_base + (INT_STS_OFFSET / sizeof(uint32_t))); /* Force a read to clear the register */
    uint32_t set_value = CNT_CTRL_STOP | CNT_CTRL_INT | CNT_CTRL_MATCH | CNT_CTRL_RST;
    timer_set_register(timer_drv, CNT_CTRL_OFFSET, set_value);
    timer_set_register(timer_drv, CLK_CTRL_OFFSET, set_value);
    set_value = INT_EN_INTERVAL;
    timer_set_register(timer_drv, INT_EN_OFFSET, set_value);
    set_value = INTERVAL_CNT_MAX;
    timer_set_register(timer_drv, INTERVAL_OFFSET, INTERVAL_CNT_MAX);

    return 0;
}
