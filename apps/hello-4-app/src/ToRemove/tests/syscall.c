/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

/* Include Kconfig variables. */
#include <autoconf.h>

#include <stdio.h>
#include <assert.h>

#include <sel4/sel4.h>
#include <vka/object.h>

#include "../helpers.h"

/*
 * Perform the codeblock given in "code", testing registers before and after to
 * ensure that cloberring does not occur.
 */
#if defined(ARCH_ARM)
#define TEST_REGISTERS(code) \
    do { \
        register int a00 = 0xdead0000; \
        register int a01 = 0xdead0001; \
        register int a02 = 0xdead0002; \
        register int a03 = 0xdead0003; \
        register int a04 = 0xdead0004; \
        register int a05 = 0xdead0005; \
        register int a06 = 0xdead0006; \
        register int a07 = 0xdead0007; \
        register int a08 = 0xdead0008; \
        register int a09 = 0xdead0009; \
        register int a10 = 0xdead000a; \
        register int a11 = 0xdead000b; \
        __asm__ __volatile__ ("" \
                : "+r"(a00), "+r"(a01), "+r"(a02), "+r"(a03), \
                  "+r"(a04), "+r"(a05), "+r"(a06), "+r"(a07), \
                  "+r"(a08), "+r"(a09), "+r"(a10), "+r"(a11)); \
        code ; \
        __asm__ __volatile__ ("" \
                : "+r"(a00), "+r"(a01), "+r"(a02), "+r"(a03), \
                  "+r"(a04), "+r"(a05), "+r"(a06), "+r"(a07), \
                  "+r"(a08), "+r"(a09), "+r"(a10), "+r"(a11)); \
        test_assert(a00 == 0xdead0000); \
        test_assert(a01 == 0xdead0001); \
        test_assert(a02 == 0xdead0002); \
        test_assert(a03 == 0xdead0003); \
        test_assert(a04 == 0xdead0004); \
        test_assert(a05 == 0xdead0005); \
        test_assert(a06 == 0xdead0006); \
        test_assert(a07 == 0xdead0007); \
        test_assert(a08 == 0xdead0008); \
        test_assert(a09 == 0xdead0009); \
        test_assert(a10 == 0xdead000a); \
        test_assert(a11 == 0xdead000b); \
    } while (0)
#elif defined(ARCH_IA32)
#define TEST_REGISTERS(code) \
    do { \
        register int a00 = 0xdead0000; \
        register int a01 = 0xdead0001; \
        register int a02 = 0xdead0002; \
        register int a03 = 0xdead0003; \
        register int a04 = 0xdead0004; \
        register int a05 = 0xdead0005; \
        register int a06 = 0xdead0006; \
        register int a07 = 0xdead0007; \
        code ; \
        __asm__ __volatile__ ("" \
                : "+r"(a00), "+r"(a01), "+r"(a02), "+r"(a03), \
                  "+r"(a04), "+r"(a05)); \
        test_assert(a00 == 0xdead0000); \
        test_assert(a01 == 0xdead0001); \
        test_assert(a02 == 0xdead0002); \
        test_assert(a03 == 0xdead0003); \
        test_assert(a04 == 0xdead0004); \
        test_assert(a05 == 0xdead0005); \
        test_assert(a06 == 0xdead0006); \
        test_assert(a07 == 0xdead0007); \
    } while (0)
#else
#error "Unknown architecture."
#endif

/* Generate a stub that tests the code "_code" with TEST_REGISTERS. */
#define GENERATE_SYSCALL_TEST(_test, _syscall, _code) \
    static int test_ ## _syscall(env_t env, void *arg) { \
        for (int i = 0; i < 10; i++) \
            TEST_REGISTERS(_code); \
        return SUCCESS; \
    } \
    DEFINE_TEST(_test, "Basic " #_syscall "() testing", test_ ## _syscall)

/*
 * Generate testing stubs for each of the basic system calls.
 */

GENERATE_SYSCALL_TEST(SYSCALL0000, seL4_Yield,
                      seL4_Yield());

GENERATE_SYSCALL_TEST(SYSCALL0001, seL4_Send,
                      seL4_Send(env->cspace_root, seL4_MessageInfo_new(0, 0, 0, 0)))

GENERATE_SYSCALL_TEST(SYSCALL0002, seL4_NBSend,
                      seL4_NBSend(env->cspace_root, seL4_MessageInfo_new(0, 0, 0, 0)))

GENERATE_SYSCALL_TEST(SYSCALL0003, seL4_Reply,
                      seL4_Reply(seL4_MessageInfo_new(0, 0, 0, 0)))

GENERATE_SYSCALL_TEST(SYSCALL0004, seL4_Notify,
                      seL4_Notify(env->cspace_root, 0))

GENERATE_SYSCALL_TEST(SYSCALL0005, seL4_Call,
                      seL4_Call(env->cspace_root, seL4_MessageInfo_new(0, 0, 0 , 0)))

static int
test_debug_put_char(env_t env, void *args)
{
    for (int i = 0; i < 10; i++) {
#ifdef CONFIG_DEBUG_BUILD
        TEST_REGISTERS(seL4_DebugPutChar(' '));
#endif
    }
    return SUCCESS;
}
DEFINE_TEST(SYSCALL0006, "Basic seL4_DebugPutChar() testing", test_debug_put_char)

/* Slightly more complex tests for waiting, because we actually have
 * to wait on something real. */
static int
test_wait(env_t env, void *args)
{
    /* Allocate an endpoint. */
    seL4_CPtr endpoint;
    endpoint = vka_alloc_async_endpoint_leaky(&env->vka);

    for (int i = 0; i < 10; i++) {
        /* Notify it, so that we don't block. */
        seL4_Notify(endpoint, 1);

        /* Wait for the endpoint. */
        TEST_REGISTERS(seL4_Wait(endpoint, NULL));
    }

    return SUCCESS;
}
DEFINE_TEST(SYSCALL0010, "Basic seL4_Wait() testing", test_wait)

static int
test_reply_wait(env_t env, void *args)
{
    /* Allocate an endpoint. */
    seL4_CPtr endpoint;
    endpoint = vka_alloc_async_endpoint_leaky(&env->vka);

    for (int i = 0; i < 10; i++) {
        /* Notify it, so that we don't block. */
        seL4_Notify(endpoint, 1);

        /* ReplyWait for the endpoint. */
        TEST_REGISTERS(seL4_ReplyWait(endpoint, seL4_MessageInfo_new(0, 0, 0, 0), NULL));
    }

    return SUCCESS;
}
DEFINE_TEST(SYSCALL0011, "Basic seL4_ReplyWait() testing", test_reply_wait)
