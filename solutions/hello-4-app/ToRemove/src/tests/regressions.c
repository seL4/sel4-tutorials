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

#include "../helpers.h"

/* This file contains tests related to bugs that have previously occured. Tests
 * should validate that the bug no longer exists.
 */

/* Previously the layout of seL4_UserContext in libsel4 has been inconsistent
 * with frameRegisters/gpRegisters in the kernel. This causes the syscalls
 * seL4_TCB_ReadRegisters and seL4_TCB_WriteRegisters to function incorrectly.
 * The following tests whether this issue has been re-introduced. For more
 * information, see SELFOUR-113.
 */

/* An endpoint for the helper thread and the main thread to synchronise on. */
static seL4_CPtr shared_endpoint;

/* This function provides a wrapper around seL4_Send to the parent thread. It
 * can't be called directly from asm because seL4_Send typically gets inlined
 * and its likely that no visible copy of this function exists to branch to.
 */
void reply_to_parent(int result)
__attribute__((noinline));
void reply_to_parent(int result)
{
    seL4_MessageInfo_t info = seL4_MessageInfo_new(result, 0, 0, 0);
    seL4_Word badge = 0; /* ignored */

    seL4_Send(shared_endpoint, info);

    /* Block to avoid returning and assume our parent will kill us. */
    (void)seL4_Wait(shared_endpoint, &badge);
}

/* Test the registers we have been setup with and pass the result back to our
 * parent. This function is really static, but GCC doesn't like a static
 * declaration when the definition is in asm.
 */
void test_registers(void)
#if defined(ARCH_ARM)
/* Probably not necessary to mark this function naked as we define the
 * whole thing in asm anyway, but just in case GCC tries to do anything
 * sneaky.
 */
__attribute__((naked))
#endif
;

/* test_registers (below) runs in a context without a valid SP. At the end it
 * needs to call into a C function and needs a valid stack to do this. We
 * provide it a stack it can switch to here.
 */
asm ("\n"
     ".bss                        \n"
     ".align  4                   \n"
     ".space 4096                 \n"
     "_safe_stack:                \n"
     ".text                       \n");
#if defined(ARCH_ARM)
asm ("\n"

     /* Trampoline for providing the thread a valid stack before entering
      * reply_to_parent. No blx required because reply_to_parent does not
      * return.
      */
     "reply_trampoline:           \n"
     "\tldr sp, =_safe_stack      \n"
     "\tb reply_to_parent         \n"

     "test_registers:             \n"
     /* Assume the PC and CPSR are correct because there's no good way of
      * testing them. Test each of the registers against the magic numbers our
      * parent set.
      */
     "\tcmp sp, #13               \n"
     "\tbne test_registers_fail   \n"
     "\tcmp r0, #15               \n"
     "\tbne test_registers_fail   \n"
     "\tcmp r1, #1                \n"
     "\tbne test_registers_fail   \n"
     "\tcmp r2, #2                \n"
     "\tbne test_registers_fail   \n"
     "\tcmp r3, #3                \n"
     "\tbne test_registers_fail   \n"
     "\tcmp r4, #4                \n"
     "\tbne test_registers_fail   \n"
     "\tcmp r5, #5                \n"
     "\tbne test_registers_fail   \n"
     "\tcmp r6, #6                \n"
     "\tbne test_registers_fail   \n"
     "\tcmp r7, #7                \n"
     "\tbne test_registers_fail   \n"
     "\tcmp r8, #8                \n"
     "\tbne test_registers_fail   \n"
     "\tcmp r9, #9                \n"
     "\tbne test_registers_fail   \n"
     "\tcmp r10, #10              \n"
     "\tbne test_registers_fail   \n"
     "\tcmp r11, #11              \n"
     "\tbne test_registers_fail   \n"
     "\tcmp r12, #12              \n"
     "\tbne test_registers_fail   \n"
     "\tcmp r14, #14              \n"
     "\tbne test_registers_fail   \n"

     /* Return success. Note that we don't bother saving registers or bl because
      * we're not planning to return here and we don't have a valid stack.
      */
     "\tmov r0, #0                \n"
     "\tb reply_trampoline        \n"

     /* Return failure. */
     "test_registers_fail:        \n"
     "\tmov r0, #1                \n"
     "\tb reply_trampoline        \n"
    );
#elif defined(ARCH_IA32)
#ifdef CONFIG_X86_64
asm ("\n\t"
     "reply_trampoline:         \n"
     "leaq  _safe_stack, %rsp   \n"
     "movq  %rax, %rdi          \n"
     "call  reply_to_parent     \n"
     "test_registers:           \n"
     "jb    rflags_ok           \n"
     "jmp   test_registers_fail \n"
     "rflags_ok:                \n"
     "cmpq  $0x0000000a, %rax   \n"
     "jne   test_registers_fail \n"
     "movq  $2, %rax\n"
     "cmpq  $0x0000000b, %rbx   \n"
     "jne   test_registers_fail \n"
     "movq  $3, %rax\n"
     "cmpq  $0x0000000c, %rcx   \n"
     "jne   test_registers_fail \n"
     "movq  $4, %rax\n"
     "cmpq  $0x0000000d, %rdx   \n"
     "jne   test_registers_fail \n"
     "movq  $5, %rax\n"
     "cmpq  $0x00000005, %rsi   \n"
     "jne   test_registers_fail \n"
     "movq  $6, %rax\n"
     "cmpq  $0x00000002, %rdi   \n"
     "jne   test_registers_fail \n"
     "movq  $7, %rax\n"
     "cmpq  $0x00000003, %rbp   \n"
     "jne   test_registers_fail \n"
     "movq  $8, %rax            \n"
     "cmpq  $0x00000004, %rsp   \n"
     "jne   test_registers_fail \n"
     "movq  $9, %rax\n"
     "cmpq  $0x00000088, %r8    \n"
     "jne   test_registers_fail \n"
     "movq  $100, %rax\n"
     "cmpq  $0x00000099, %r9    \n"
     "jne   test_registers_fail \n"
     "movq  $11, %rax\n"
     "cmpq  $0x00000010, %r10   \n"
     "jne   test_registers_fail \n"
     "movq  $12, %rax\n"
     "cmpq  $0x00000011, %r11   \n"
     "jne   test_registers_fail \n"
     "movq  $13, %rax\n"
     "cmpq  $0x00000012, %r12   \n"
     "jne   test_registers_fail \n"
     "movq  $14, %rax\n"
     "cmpq  $0x00000013, %r13   \n"
     "jne   test_registers_fail \n"
     "movq  $15, %rax\n"
     "cmpq  $0x00000014, %r14   \n"
     "jne   test_registers_fail \n"
     "movq  $16, %rax\n"
     "cmpq  $0x00000015, %r15   \n"
     "jne   test_registers_fail \n"
     "movq  $0, %rax            \n"
     "jmp   reply_trampoline    \n"
     "test_registers_fail:      \n"
     "movq  $1, %rax            \n"
     "jmp   reply_trampoline    \n"
    );
#else
asm ("\n"

     /* As for the ARM implementation above, but we also need to massage the
      * calling convention by taking the value test_registers passed us in EAX
      * and put it on the stack where reply_to_parent expects it.
      */
     "reply_trampoline:           \n"
     "\tleal _safe_stack, %esp    \n"
     "\tpushl %eax                \n"
     "\tcall reply_to_parent      \n"

     "test_registers:             \n"
     /* Assume EIP, GS and FS are correct. Is there a good way to
      * test these?
      *  EIP - If this is incorrect we'll never arrive at this function.
      *  GS - With an incorrect GDT selector we fault and die immediately.
      *  FS - Overwritten by the kernel before we jump here.
      */

     /* We need to test EFLAGS indirectly because we can't cmp it. The jb
      * should only be taken if CF (bit 0) is set.
      */
     "\tjb eflags_ok              \n"
     "\tjmp test_registers_fail   \n"
     "eflags_ok:                  \n"
     "\tcmpl $0x0000000a, %eax    \n"
     "\tjne test_registers_fail   \n"
     "\tcmpl $0x0000000b, %ebx    \n"
     "\tjne test_registers_fail   \n"
     "\tcmpl $0x0000000c, %ecx    \n"
     "\tjne test_registers_fail   \n"
     "\tcmpl $0x0000000d, %edx    \n"
     "\tjne test_registers_fail   \n"
     "\tcmpl $0x00000005, %esi    \n"
     "\tjne test_registers_fail   \n"
     "\tcmpl $0x00000002, %edi    \n"
     "\tjne test_registers_fail   \n"
     "\tcmpl $0x00000003, %ebp    \n"
     "\tjne test_registers_fail   \n"
     "\tcmpl $0x00000004, %esp    \n"
     "\tjne test_registers_fail   \n"

     /* Return success. Note we use a custom calling convention because we
      * don't have a valid stack.
      */
     "\tmovl $0, %eax             \n"
     "\tjmp reply_trampoline      \n"

     /* Return failure. */
     "test_registers_fail:        \n"
     "\tmovl $1, %eax             \n"
     "\tjmp reply_trampoline      \n"
    );
#endif
#else
#error "Unsupported architecture"
#endif
int test_write_registers(env_t env, void *arg)
{
    helper_thread_t thread;
    seL4_UserContext context = { 0 };
    int result;
    seL4_MessageInfo_t info;
    seL4_Word badge = 0; /* ignored */

    /* Create a thread without starting it. Most of these arguments are
     * ignored.
     */
    create_helper_thread(env, &thread);
    shared_endpoint = thread.local_endpoint.cptr;

#if defined(ARCH_ARM)
    context.pc = (seL4_Word)&test_registers;
    context.sp = 13;
    context.r0 = 15;
    context.r1 = 1;
    context.r2 = 2;
    context.r3 = 3;
    context.r4 = 4;
    context.r5 = 5;
    context.r6 = 6;
    context.r7 = 7;
    context.r8 = 8;
    context.r9 = 9;
    context.r10 = 10;
    context.r11 = 11;
    context.r12 = 12;
    /* R13 == SP */
    context.r14 = 14; /* LR */
    /* R15 == PC */
#elif defined(ARCH_IA32)
#ifdef CONFIG_X86_64
    context.rip = (seL4_Word)&test_registers;
    context.rsp = 0x00000004UL;
    context.gs  = IPCBUF_GDT_SELECTOR;
    context.rax = 0x0000000aUL;
    context.rbx = 0x0000000bUL;
    context.rcx = 0x0000000cUL;
    context.rdx = 0x0000000dUL;
    context.rsi = 0x00000005UL;
    context.rdi = 0x00000002UL;
    context.rbp = 0x00000003UL;
    context.rflags = 0x00000001UL;
    context.r8 = 0x00000088UL;
    context.r9 = 0x00000099UL;
    context.r10 = 0x00000010UL;
    context.r11 = 0x00000011UL;
    context.r12 = 0x00000012UL;
    context.r13 = 0x00000013UL;
    context.r14 = 0x00000014UL;
    context.r15 = 0x00000015UL;
#else
    context.eip = (seL4_Word)&test_registers;
    context.esp = 0x00000004;
    context.gs = IPCBUF_GDT_SELECTOR;
    context.eax = 0x0000000a;
    context.ebx = 0x0000000b;
    context.ecx = 0x0000000c;
    context.edx = 0x0000000d;
    context.esi = 0x00000005;
    context.edi = 0x00000002;
    context.ebp = 0x00000003;
    context.eflags = 0x00000001; /* Set the CF bit */
#endif
#else
#error "Unsupported architecture"
#endif

    result = seL4_TCB_WriteRegisters(thread.thread.tcb.cptr, true, 0 /* ignored */,
                                     sizeof(seL4_UserContext) / sizeof(seL4_Word), &context);

    if (!result) {
        /* If we've successfully started the thread, block until it's checked
         * its registers.
         */
        info = seL4_Wait(shared_endpoint, &badge);
    }
    cleanup_helper(env, &thread);

    test_assert(result == 0);

    result = seL4_MessageInfo_get_label(info);
    test_assert(result == 0);

    return SUCCESS;
}
DEFINE_TEST(REGRESSIONS0001, "Ensure WriteRegisters functions correctly", test_write_registers)

#ifdef ARCH_ARM
/* Performs an ldrex and strex sequence with a context switch in between. See
 * the comment in the function following for an explanation of purpose.
 */
static int do_ldrex(void)
{
    seL4_Word dummy1, dummy2, result;

    /* We don't really care where we are loading from here. This is just to set
     * the exclusive access tag.
     */
    asm volatile ("ldrex %[rt], [%[rn]]"
                  : [rt]"=&r"(dummy1)
                  : [rn]"r"(&dummy2));

    /* Force a context switch to our parent. */
    seL4_Notify(shared_endpoint, 0);

    /* Again, we don't care where we are storing to. This is to see whether the
     * exclusive access tag is still set.
     */
    asm volatile ("strex %[rd], %[rt], [%[rn]]"
                  : [rd]"=&r"(result)
                  : [rt]"r"(dummy2), [rn]"r"(&dummy1));

    /* The strex should have failed (and returned 1) because the context switch
     * should have cleared the exclusive access tag.
     */
    return result == 0 ? FAILURE : SUCCESS;
}

/* Prior to kernel changeset a4656bf3066e the load-exclusive monitor was not
 * cleared on a context switch. This causes unexpected and incorrect behaviour
 * for any userspace program relying on ldrex/strex to implement exclusion
 * mechanisms. This test checks that the monitor is cleared correctly on
 * switch. See SELFOUR-141 for more information.
 */
int test_ldrex_cleared(env_t env, void *arg)
{
    helper_thread_t thread;
    seL4_Word result;
    seL4_Word badge = 0; /* ignored */

    /* Create a child to perform the ldrex/strex. */
    create_helper_thread(env, &thread);
    shared_endpoint = thread.local_endpoint.cptr;
    start_helper(env, &thread, (helper_fn_t) do_ldrex, 0, 0, 0, 0);

    /* Wait for the child to do ldrex and signal us. */
    seL4_Wait(shared_endpoint, &badge);

    /* Wait for the child to do strex and exit. */
    result = wait_for_helper(&thread);

    cleanup_helper(env, &thread);

    return result;
}
DEFINE_TEST(REGRESSIONS0002, "Test the load-exclusive monitor is cleared on context switch", test_ldrex_cleared)
#endif
