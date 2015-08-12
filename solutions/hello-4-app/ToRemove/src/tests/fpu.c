/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

#include "../helpers.h"

/*
 * Ensure basic FPU functionality works.
 *
 * For processors without a FPU, this tests that maths libraries link
 * correctly.
 */
static int
test_fpu_trivial(env_t env, void *arg)
{
    int i;
    volatile double b;
    double a = (double)3.141592653589793238462643383279;

    for (i = 0; i < 100; i++) {
        a = a * 3 + (a / 3);
    }
    b = a;
    (void)b;
    return SUCCESS;
}
DEFINE_TEST(FPU0000, "Ensure that simple FPU operations work", test_fpu_trivial)

static int
fpu_worker(seL4_Word p1, seL4_Word p2, seL4_Word p3, seL4_Word p4)
{
    volatile double *state = (volatile double *)p1;
    int num_iterations = p2;
    static volatile int preemption_counter = 0;
    int num_preemptions = 0;

    while (num_iterations >= 0) {
        int counter_init = preemption_counter;

        /* Do some random calculation (where we know the result). */
        int i;
        double a = (double)3.141;
        for (i = 0; i < 10000; i++) {
            a = a * 1.123 + (a / 3);
            a /= 1.111;
            while (a > 100.0) {
                a /= 3.1234563212;
            }
            while (a < 2.0) {
                a += 1.1232132131;
            }
        }
        *state = a;
        a = *state;

        /* Determine if we were preempted mid-calculation. */
        if (counter_init != preemption_counter) {
            num_preemptions++;
        }
        preemption_counter++;

        num_iterations--;
    }

    return num_preemptions;
}

/*
 * Test that multiple threads using the FPU simulataneously can't corrupt each
 * other.
 *
 * Early versions of seL4 had a bug here because we were not context-switching
 * the FPU at all. Oops.
 */
#ifdef CONFIG_X86_64
#define NUM_THREADS  4
static helper_thread_t thread[NUM_THREADS];
static volatile double thread_state[NUM_THREADS];
#endif

#ifndef CONFIG_FT

static int
test_fpu_multithreaded(struct env* env, void *args)
{
#ifndef CONFIG_X86_64
    const int NUM_THREADS = 4;
    helper_thread_t thread[NUM_THREADS];
    volatile double thread_state[NUM_THREADS];
#endif
    seL4_Word iterations = 1;
    int num_preemptions = 0;

    /*
     * We keep increasing the number of iterations ours users should calculate
     * for until they notice themselves being preempted a few times.
     */
    do {
        /* Start the threads running. */
        for (int i = 0; i < NUM_THREADS; i++) {
            create_helper_thread(env, &thread[i]);
            set_helper_priority(&thread[i], 100);
            start_helper(env, &thread[i], fpu_worker,
                         (seL4_Word) &thread_state[i], iterations, 0, 0);
        }

        /* Wait for the threads to finish. */
        num_preemptions = 0;
        for (int i = 0; i < NUM_THREADS; i++) {
            num_preemptions += wait_for_helper(&thread[i]);
            cleanup_helper(env, &thread[i]);
        }

        /* Ensure they all got the same result. An assert failure here
         * indicates FPU corrupt (i.e., a kernel bug). */
        for (int i = 0; i < NUM_THREADS; i++) {
            test_assert(thread_state[i] == thread_state[(i + 1) % NUM_THREADS]);
        }

        /* If we didn't get enough preemptions, restart everything again. */
        iterations *= 2;
    } while (num_preemptions < 20);

    return SUCCESS;
}
DEFINE_TEST(FPU0001, "Ensure multiple threads can use FPU simultaneously", test_fpu_multithreaded)

#endif
