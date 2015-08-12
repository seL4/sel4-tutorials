/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <sel4/sel4.h>
#include <sel4/messages.h>
#include <sel4utils/arch/util.h>

#include <vka/object.h>

#include "../test.h"
#include "../helpers.h"

enum {
    FAULT_DATA_READ_PAGEFAULT = 1,
    FAULT_DATA_WRITE_PAGEFAULT = 2,
    FAULT_INSTRUCTION_PAGEFAULT = 3,
    FAULT_BAD_SYSCALL = 4,
    FAULT_BAD_INSTRUCTION = 5,
};

#define BAD_VADDR 0xf123456C
#define GOOD_MAGIC 0x15831851
#define BAD_MAGIC ~GOOD_MAGIC
#define BAD_SYSCALL_NUMBER 0xc1

#define EXPECTED_BADGE 0xabababa

extern char read_fault_address[];
extern char read_fault_restart_address[];
static void __attribute__((noinline))
do_read_fault(void)
{
    int *x = (int*)BAD_VADDR;
    int val = BAD_MAGIC;
    /* Do a read fault. */
#if defined(ARCH_ARM)
    asm volatile (
        "mov r0, %[val]\n\t"
        "read_fault_address:\n\t"
        "ldr r0, [%[addrreg]]\n\t"
        "read_fault_restart_address:\n\t"
        "mov %[val], r0\n\t"
        : [val] "+r" (val)
        : [addrreg] "r" (x)
        : "r0"
    );
#elif defined(ARCH_IA32)
    asm volatile (
        "mov %[val], %%eax\n\t"
        "read_fault_address:\n\t"
        "mov (%[addrreg]), %%eax\n\t"
        "read_fault_restart_address:\n\t"
        "mov %%eax, %[val]\n\t"
        : [val] "+r" (val)
        : [addrreg] "r" (x)
        : "eax"
    );
#else
#error "Unknown architecture."
#endif
    test_check(val == GOOD_MAGIC);
}

extern char write_fault_address[];
extern char write_fault_restart_address[];
static void __attribute__((noinline))
do_write_fault(void)
{
    int *x = (int*)BAD_VADDR;
    int val = BAD_MAGIC;
    /* Do a write fault. */
#if defined(ARCH_ARM)
    asm volatile (
        "mov r0, %[val]\n\t"
        "write_fault_address:\n\t"
        "str r0, [%[addrreg]]\n\t"
        "write_fault_restart_address:\n\t"
        "mov %[val], r0\n\t"
        : [val] "+r" (val)
        : [addrreg] "r" (x)
        : "r0"
    );
#elif defined(ARCH_IA32)
    asm volatile (
        "mov %[val], %%eax\n\t"
        "write_fault_address:\n\t"
        "mov %%eax, (%[addrreg])\n\t"
        "write_fault_restart_address:\n\t"
        "mov %%eax, %[val]\n\t"
        : [val] "+r" (val)
        : [addrreg] "r" (x)
        : "eax"
    );
#else
#error "Unknown architecture."
#endif
    test_check(val == GOOD_MAGIC);
}

extern char instruction_fault_restart_address[];
static void __attribute__((noinline))
do_instruction_fault(void)
{
    int *x = (int*)BAD_VADDR;
    int val = BAD_MAGIC;
    /* Jump to a crazy address. */
#if defined(ARCH_ARM)
    asm volatile (
        "mov r0, %[val]\n\t"
        "blx %[addrreg]\n\t"
        "instruction_fault_restart_address:\n\t"
        "mov %[val], r0\n\t"
        : [val] "+r" (val)
        : [addrreg] "r" (x)
        : "r0", "lr"
    );
#elif defined(ARCH_IA32)
    asm volatile (
        "mov %[val], %%eax\n\t"
        "instruction_fault_address:\n\t"
        "jmp *%[addrreg]\n\t"
        "instruction_fault_restart_address:\n\t"
        "mov %%eax, %[val]\n\t"
        : [val] "+r" (val)
        : [addrreg] "r" (x)
        : "eax"
    );
#else
#error "Unknown architecture."
#endif
    test_check(val == GOOD_MAGIC);
}

extern char bad_syscall_address[];
extern char bad_syscall_restart_address[];
static void __attribute__((noinline))
do_bad_syscall(void)
{
    int *x = (int*)BAD_VADDR;
    int val = BAD_MAGIC;
    /* Do an undefined system call. */
#if defined(ARCH_ARM)
    asm volatile (
        "mov r7, %[scno]\n\t"
        "mov r0, %[val]\n\t"
        "bad_syscall_address:\n\t"
        "svc %[scno]\n\t"
        "bad_syscall_restart_address:\n\t"
        "mov %[val], r0\n\t"
        : [val] "+r" (val)
        : [addrreg] "r" (x),
        [scno] "i" (BAD_SYSCALL_NUMBER)
        : "r0", "r1", "r2", "r3", "r4", "r5", "r6", "r7", "memory", "cc"
    );
#elif defined(ARCH_IA32)
#ifdef CONFIG_X86_64
    asm volatile (
        "movl   %[val], %%eax\n\t"
        "movq   %%rsp, %%rcx\n\t"
        "leaq   1f, %%rdx\n\t"
        "bad_syscall_address:\n\t"
        "1: \n\t"
        "sysenter\n\t"
        "bad_syscall_restart_address:\n\t"
        "movl   %%eax, %[val]\n\t"
        : [val] "+r" (val)
        : [addrreg] "r" (x),
        [scno] "i" (BAD_SYSCALL_NUMBER)
        : "rax", "rbx", "rcx", "rdx"
    );
#else
    asm volatile (
        "mov %[val], %%eax\n\t"
        "mov %%esp, %%ecx\n\t"
        "leal 1f, %%edx\n\t"
        "bad_syscall_address:\n\t"
        "1:\n\t"
        "sysenter\n\t"
        "bad_syscall_restart_address:\n\t"
        "mov %%eax, %[val]\n\t"
        : [val] "+r" (val)
        : [addrreg] "r" (x),
        [scno] "i" (BAD_SYSCALL_NUMBER)
        : "eax", "ebx", "ecx", "edx"
    );
#endif
#else
#error "Unknown architecture."
#endif
    test_check(val == GOOD_MAGIC);
}

extern char bad_instruction_address[];
extern char bad_instruction_restart_address[];
static seL4_Word bad_instruction_sp; /* To reset afterwards. */
static seL4_Word bad_instruction_cpsr; /* For checking against. */
static void __attribute__((noinline))
do_bad_instruction(void)
{
    int val = BAD_MAGIC;
    /* Execute an undefined instruction. */
#if defined(ARCH_ARM)
    asm volatile (
        /* Save SP */
        "str sp, [%[sp]]\n\t"

        /* Save CPSR */
        "mrs r0, cpsr\n\t"
        "str r0, [%[cpsr]]\n\t"

        /* Set SP to val. */
        "mov sp, %[valptr]\n\t"

        "bad_instruction_address:\n\t"
        ".word 0xe7f000f0\n\t" /* Guaranteed to be undefined by ARM. */
        "bad_instruction_restart_address:\n\t"
        :
        : [sp] "r" (&bad_instruction_sp),
        [cpsr] "r" (&bad_instruction_cpsr),
        [valptr] "r" (&val)
        : "r0", "memory"
    );
#elif defined(ARCH_IA32)
#ifdef CONFIG_X86_64
    asm volatile (
        /* save RSP */
        "movq   %%rsp, (%[sp])\n\t"
        "pushf\n\t"
        "pop    %%rax\n\t"
        "movq   %%rax, (%[cpsr])\n\t"
        "movq   %[valptr], %%rsp\n\t"
        "bad_instruction_address:\n\t"
        "ud2\n\t"
        "bad_instruction_restart_address:\n\t"
        :
        : [sp] "r" (&bad_instruction_sp),
        [cpsr] "r" (&bad_instruction_cpsr),
        [valptr] "r" (&val)
        : "rax", "memory"
    );
#else
    asm volatile (
        /* Save SP */
        "mov %%esp, (%[sp])\n\t"

        /* Save CPSR */
        "pushf\n\t"
        "pop %%eax\n\t"
        "mov %%eax, (%[cpsr])\n\t"

        /* Set SP to val. */
        "mov %[valptr], %%esp\n\t"

        "bad_instruction_address:\n\t"
        "ud2\n\t"
        "bad_instruction_restart_address:\n\t"
        :
        : [sp] "r" (&bad_instruction_sp),
        [cpsr] "r" (&bad_instruction_cpsr),
        [valptr] "r" (&val)
        : "eax", "memory"
    );
#endif
#else
#error "Unknown architecture."
#endif
    test_check(val == GOOD_MAGIC);
}

static void __attribute__((noinline))
set_good_magic_and_set_pc(seL4_CPtr tcb, seL4_Word new_pc)
{
    /* Set their register to GOOD_MAGIC and set PC past fault. */
    int error;
    seL4_UserContext ctx;
    error = seL4_TCB_ReadRegisters(tcb,
                                   false,
                                   0,
                                   sizeof(ctx) / sizeof(seL4_Word),
                                   &ctx);
    test_assert_fatal(!error);
#if defined(ARCH_ARM)
    test_assert_fatal(ctx.r0 == BAD_MAGIC);
    ctx.r0 = GOOD_MAGIC;
    ctx.pc = new_pc;
#elif defined(ARCH_IA32)
#ifdef CONFIG_X86_64
    test_assert_fatal((int)ctx.rax == BAD_MAGIC);
    ctx.rax = GOOD_MAGIC;
    ctx.rip = new_pc;
#else
    test_assert_fatal(ctx.eax == BAD_MAGIC);
    ctx.eax = GOOD_MAGIC;
    ctx.eip = new_pc;
#endif
#else
#error "Unknown architecture."
#endif
    error = seL4_TCB_WriteRegisters(tcb,
                                    false,
                                    0,
                                    sizeof(ctx) / sizeof(seL4_Word),
                                    &ctx);
    test_assert_fatal(!error);
}

static int
handle_fault(seL4_CPtr fault_ep, seL4_CPtr tcb, seL4_Word expected_fault,
             int badged_or_restart)
{
    seL4_MessageInfo_t tag;
    seL4_Word sender_badge = 0;
    seL4_Word badged = !!(badged_or_restart & 2);
    seL4_Word restart = !!(badged_or_restart & 1);

    tag = seL4_Wait(fault_ep, &sender_badge);

    if (badged) {
        test_check(sender_badge == EXPECTED_BADGE);
    } else {
        test_check(sender_badge == 0);
    }

    switch (expected_fault) {
    case FAULT_DATA_READ_PAGEFAULT:
        test_check(seL4_MessageInfo_get_label(tag) == SEL4_PFIPC_LABEL);
        test_check(seL4_MessageInfo_get_length(tag) == SEL4_PFIPC_LENGTH);
        test_check(seL4_GetMR(SEL4_PFIPC_FAULT_IP) == (seL4_Word)read_fault_address);
        test_check(seL4_GetMR(SEL4_PFIPC_FAULT_ADDR) == BAD_VADDR);
        test_check(seL4_GetMR(SEL4_PFIPC_PREFETCH_FAULT) == 0);
        test_check(sel4utils_is_read_fault());

        /* Clear MRs to ensure they get repopulated. */
        seL4_SetMR(SEL4_PFIPC_FAULT_ADDR, 0);

        set_good_magic_and_set_pc(tcb, (seL4_Word)read_fault_restart_address);
        if (restart) {
            seL4_Reply(tag);
        }
        break;

    case FAULT_DATA_WRITE_PAGEFAULT:
        test_check(seL4_MessageInfo_get_label(tag) == SEL4_PFIPC_LABEL);
        test_check(seL4_MessageInfo_get_length(tag) == SEL4_PFIPC_LENGTH);
        test_check(seL4_GetMR(SEL4_PFIPC_FAULT_IP) == (seL4_Word)write_fault_address);
        test_check(seL4_GetMR(SEL4_PFIPC_FAULT_ADDR) == BAD_VADDR);
        test_check(seL4_GetMR(SEL4_PFIPC_PREFETCH_FAULT) == 0);
        test_check(!sel4utils_is_read_fault());

        /* Clear MRs to ensure they get repopulated. */
        seL4_SetMR(SEL4_PFIPC_FAULT_ADDR, 0);

        set_good_magic_and_set_pc(tcb, (seL4_Word)write_fault_restart_address);
        if (restart) {
            seL4_Reply(tag);
        }
        break;

    case FAULT_INSTRUCTION_PAGEFAULT:
        test_check(seL4_MessageInfo_get_label(tag) == SEL4_PFIPC_LABEL);
        test_check(seL4_MessageInfo_get_length(tag) == SEL4_PFIPC_LENGTH);
        test_check(seL4_GetMR(SEL4_PFIPC_FAULT_IP) == BAD_VADDR);
        test_check(seL4_GetMR(SEL4_PFIPC_FAULT_ADDR) == BAD_VADDR);
#if defined(ARCH_ARM)
        /* Prefetch fault is only set on ARM. */
        test_check(seL4_GetMR(SEL4_PFIPC_PREFETCH_FAULT) == 1);
#endif
        test_check(sel4utils_is_read_fault());

        /* Clear MRs to ensure they get repopulated. */
        seL4_SetMR(SEL4_PFIPC_FAULT_ADDR, 0);

        set_good_magic_and_set_pc(tcb, (seL4_Word)instruction_fault_restart_address);
        if (restart) {
            seL4_Reply(tag);
        }
        break;

    case FAULT_BAD_SYSCALL:
        test_check(seL4_MessageInfo_get_label(tag) == SEL4_EXCEPT_IPC_LABEL);
        test_check(seL4_MessageInfo_get_length(tag) == SEL4_EXCEPT_IPC_LENGTH);
#if defined(ARCH_ARM)
        test_check(seL4_GetMR(EXCEPT_IPC_SYS_MR_R0) == BAD_MAGIC);
        test_check(seL4_GetMR(EXCEPT_IPC_SYS_MR_PC) == (seL4_Word)bad_syscall_address);
        test_check(seL4_GetMR(EXCEPT_IPC_SYS_MR_SYSCALL) == BAD_SYSCALL_NUMBER);

        seL4_SetMR(EXCEPT_IPC_SYS_MR_R0, GOOD_MAGIC);
        seL4_SetMR(EXCEPT_IPC_SYS_MR_PC, (seL4_Word)bad_syscall_restart_address);
#elif defined(ARCH_IA32)
#ifdef CONFIG_X86_64
        test_check((int)seL4_GetMR(EXCEPT_IPC_SYS_MR_RAX) == BAD_MAGIC);
        test_check(seL4_GetMR(EXCEPT_IPC_SYS_MR_RIP) == (seL4_Word)bad_syscall_restart_address);
        seL4_SetMR(EXCEPT_IPC_SYS_MR_RAX, GOOD_MAGIC);
#else
        test_check(seL4_GetMR(EXCEPT_IPC_SYS_MR_EAX) == BAD_MAGIC);
        test_check(seL4_GetMR(EXCEPT_IPC_SYS_MR_EIP) == (seL4_Word)bad_syscall_restart_address);

        seL4_SetMR(EXCEPT_IPC_SYS_MR_EAX, GOOD_MAGIC);
#endif
        /* Syscalls on ia32 seem to restart themselves with sysenter. */
#else
#error "Unknown architecture."
#endif

        /* Flag that the thread should be restarted. */
        if (restart) {
            seL4_MessageInfo_ptr_set_label(&tag, 0);
        } else {
            seL4_MessageInfo_ptr_set_label(&tag, 1);
        }

        seL4_Reply(tag);
        break;

    case FAULT_BAD_INSTRUCTION:
        test_check(seL4_MessageInfo_get_label(tag) == SEL4_USER_EXCEPTION_LABEL);
        test_check(seL4_MessageInfo_get_length(tag) == SEL4_USER_EXCEPTION_LENGTH);
        test_check(seL4_GetMR(0) == (seL4_Word)bad_instruction_address);
        seL4_Word *valptr = (seL4_Word*)seL4_GetMR(1);
        test_check((int)*valptr == BAD_MAGIC);
#if defined(ARCH_ARM)
        test_check(seL4_GetMR(2) == bad_instruction_cpsr);
        test_check(seL4_GetMR(3) == 0);
        test_check(seL4_GetMR(4) == 0);
#elif defined(ARCH_IA32)
        /*
         * Curiously, the "resume flag" (bit 16) is set between the
         * undefined syscall and seL4 grabbing the tasks's flags. This only
         * happens on x86 hardware, but not qemu. Just ignore it when
         * checking flags.
         */
        seL4_Word mask_out = ~(1 << 16);
        test_check(((seL4_GetMR(2) ^ bad_instruction_cpsr) & mask_out) == 0);
#else
#error "Unknown architecture."
#endif

        *valptr = GOOD_MAGIC;
        seL4_SetMR(0, (seL4_Word)bad_instruction_restart_address);
        seL4_SetMR(1, bad_instruction_sp);

        /* Flag that the thread should be restarted. */
        if (restart) {
            seL4_MessageInfo_ptr_set_label(&tag, 0);
        } else {
            seL4_MessageInfo_ptr_set_label(&tag, 1);
        }

        seL4_Reply(tag);
        break;


    default:
        /* What? Why are we here? What just happened? */
        test_assert_fatal(0);
        break;
    }

    return 0;
}

static int
cause_fault(int fault_type)
{
    switch (fault_type) {
    case FAULT_DATA_READ_PAGEFAULT:
        do_read_fault();
        break;
    case FAULT_DATA_WRITE_PAGEFAULT:
        do_write_fault();
        break;
    case FAULT_INSTRUCTION_PAGEFAULT:
        do_instruction_fault();
        break;
    case FAULT_BAD_SYSCALL:
        do_bad_syscall();
        break;
    case FAULT_BAD_INSTRUCTION:
        do_bad_instruction();
        break;
    }

    return 0;
}

static int
test_fault(env_t env, int fault_type, bool inter_as)
{
    helper_thread_t handler_thread;
    helper_thread_t faulter_thread;
    int error;

    for (int restart = 0; restart <= 1; restart++) {
        for (int prio = 100; prio <= 102; prio++) {
            for (int badged = 0; badged <= 1; badged++) {
                seL4_Word handler_arg0, handler_arg1;
                /* The endpoint on which faults are received. */
                seL4_CPtr fault_ep = vka_alloc_endpoint_leaky(&env->vka);
                if (badged) {
                    seL4_CPtr badged_fault_ep = get_free_slot(env);
                    seL4_CapData_t cap_data;
                    cap_data = seL4_CapData_Badge_new(EXPECTED_BADGE);
                    cnode_mint(env, fault_ep, badged_fault_ep, seL4_AllRights, cap_data);

                    fault_ep = badged_fault_ep;
                }

                seL4_CPtr faulter_vspace, faulter_cspace;

                if (inter_as) {
                    create_helper_process(env, &faulter_thread);
                    create_helper_process(env, &handler_thread);

                    /* copy the fault endpoint to the faulter */
                    cspacepath_t path;
                    vka_cspace_make_path(&env->vka,  fault_ep, &path);
                    fault_ep = sel4utils_copy_cap_to_process(&faulter_thread.process, path);
                    assert(fault_ep != -1);

                    /* copy the fault endpoint to the handler */
                    handler_arg0 = sel4utils_copy_cap_to_process(&handler_thread.process, path);
                    assert(handler_arg0 != -1);

                    /* copy the fault tcb to the handler */
                    vka_cspace_make_path(&env->vka, faulter_thread.thread.tcb.cptr, &path);
                    handler_arg1 = sel4utils_copy_cap_to_process(&handler_thread.process, path);
                    assert(handler_arg1 != -1);

                    faulter_cspace = faulter_thread.process.cspace.cptr;
                    faulter_vspace = faulter_thread.process.pd.cptr;
                } else {
                    create_helper_thread(env, &faulter_thread);
                    create_helper_thread(env, &handler_thread);
                    faulter_cspace = env->cspace_root;
                    faulter_vspace = env->page_directory;
                    handler_arg0 = fault_ep;
                    handler_arg1 = faulter_thread.thread.tcb.cptr;
                }

                set_helper_priority(&handler_thread, 101);

                error = seL4_TCB_Configure(faulter_thread.thread.tcb.cptr,
                                           fault_ep,
                                           prio,
                                           faulter_cspace,
                                           seL4_CapData_Guard_new(0, seL4_WordBits - env->cspace_size_bits),
                                           faulter_vspace, seL4_NilData,
                                           faulter_thread.thread.ipc_buffer_addr,
                                           faulter_thread.thread.ipc_buffer);
                test_assert(!error);

                start_helper(env, &handler_thread, (helper_fn_t) handle_fault,
                             handler_arg0, handler_arg1, fault_type, (badged << 1) | restart);
                start_helper(env, &faulter_thread, (helper_fn_t) cause_fault,
                             fault_type, 0, 0, 0);
                wait_for_helper(&handler_thread);

                if (restart) {
                    wait_for_helper(&faulter_thread);
                }

                cleanup_helper(env, &handler_thread);
                cleanup_helper(env, &faulter_thread);
            }
        }
    }

    return SUCCESS;
}

#ifndef CONFIG_FT

static int test_read_fault(env_t env, void *args)
{
    return test_fault(env, FAULT_DATA_READ_PAGEFAULT, false);
}
DEFINE_TEST(PAGEFAULT0001, "Test read page fault", test_read_fault)

static int test_write_fault(env_t env, void *args)
{
    return test_fault(env, FAULT_DATA_WRITE_PAGEFAULT, false);
}
DEFINE_TEST(PAGEFAULT0002, "Test write page fault", test_write_fault)

static int test_execute_fault(env_t env, void *args)
{
    return test_fault(env,  FAULT_INSTRUCTION_PAGEFAULT, false);
}
DEFINE_TEST(PAGEFAULT0003, "Test execute page fault", test_execute_fault)

#endif

static int test_bad_syscall(env_t env, void *args)
{
    return test_fault(env, FAULT_BAD_SYSCALL, false);
}
DEFINE_TEST(PAGEFAULT0004, "Test unknown system call", test_bad_syscall)

static int test_bad_instruction(env_t env, void *args)
{
    return test_fault(env, FAULT_BAD_INSTRUCTION, false);
}
DEFINE_TEST(PAGEFAULT0005, "Test undefined instruction", test_bad_instruction)

static int test_read_fault_interas(env_t env, void *args)
{
    return test_fault(env, FAULT_DATA_READ_PAGEFAULT, true);
}
DEFINE_TEST(PAGEFAULT1001, "Test read page fault (inter-AS)", test_read_fault_interas)

static int test_write_fault_interas(env_t env, void *args)
{
    return test_fault(env, FAULT_DATA_WRITE_PAGEFAULT, true);
}
DEFINE_TEST(PAGEFAULT1002, "Test write page fault (inter-AS)", test_write_fault_interas)

static int test_execute_fault_interas(env_t env, void *args)
{
    return test_fault(env, FAULT_INSTRUCTION_PAGEFAULT, true);
}
DEFINE_TEST(PAGEFAULT1003, "Test execute page fault (inter-AS)", test_execute_fault_interas)

static int test_bad_syscall_interas(env_t env, void *args)
{
    return test_fault(env, FAULT_BAD_SYSCALL, true);
}
DEFINE_TEST(PAGEFAULT1004, "Test unknown system call (inter-AS)", test_bad_syscall_interas)

#if 0
/* This test needs some work. */
static int test_bad_instruction_interas(env_t env, void *args)
{
    return test_fault(&env->vka, FAULT_BAD_INSTRUCTION, true);
}
DEFINE_TEST(PAGEFAULT1005, "Test undefined instruction (inter-AS)", test_bad_instruction_interas)
#endif
