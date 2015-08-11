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
#include <sel4utils/arch/util.h>

#include <sel4test/test.h>
#include <stdarg.h>
#include <stdlib.h>

#include <utils/util.h>
#include <vka/capops.h>

#include "helpers.h"
#include "test.h"

int
check_zeroes(seL4_Word addr, seL4_Word size_bytes)
{
    test_assert_fatal(IS_ALIGNED(addr, sizeof(seL4_Word)));
    test_assert_fatal(IS_ALIGNED(size_bytes, sizeof(seL4_Word)));
    seL4_Word *p = (void*)addr;
    seL4_Word size_words = size_bytes / sizeof(seL4_Word);
    while (size_words--) {
        if (*p++ != 0) {
            LOG_ERROR("Found non-zero at position %d: %d\n", ((int)p) - (addr), p[-1]);
            return 0;
        }
    }
    return 1;
}

/* Determine whether a given slot in the init thread's CSpace is empty by
 * examining the error when moving a slot onto itself.
 *
 * Serves as == 0 comparator for caps.
 */
int
is_slot_empty(env_t env, seL4_Word slot)
{
    int error;

    error = cnode_move(env, slot, slot);

    assert(error == seL4_DeleteFirst || error == seL4_FailedLookup);
    return (error == seL4_FailedLookup);
}

seL4_Word
get_free_slot(env_t env)
{
    seL4_CPtr slot;
    UNUSED int error = vka_cspace_alloc(&env->vka, &slot);
    assert(!error);
    return slot;
}


int
cnode_copy(env_t env, seL4_CPtr src, seL4_CPtr dest, seL4_Word rights)
{
    cspacepath_t src_path, dest_path;
    vka_cspace_make_path(&env->vka, src, &src_path);
    vka_cspace_make_path(&env->vka, dest, &dest_path);
    return vka_cnode_copy(&dest_path, &src_path, rights);
}



int
cnode_delete(env_t env, seL4_CPtr slot)
{
    cspacepath_t path;
    vka_cspace_make_path(&env->vka, slot, &path);
    return vka_cnode_delete(&path);
}

int
cnode_mint(env_t env, seL4_CPtr src, seL4_CPtr dest, seL4_Word rights, seL4_CapData_t badge)
{
    cspacepath_t src_path, dest_path;

    vka_cspace_make_path(&env->vka, src, &src_path);
    vka_cspace_make_path(&env->vka, dest, &dest_path);
    return vka_cnode_mint(&dest_path, &src_path, rights, badge);
}


int
cnode_move(env_t env, seL4_CPtr src, seL4_CPtr dest)
{
    cspacepath_t src_path, dest_path;

    vka_cspace_make_path(&env->vka, src, &src_path);
    vka_cspace_make_path(&env->vka, dest, &dest_path);
    return vka_cnode_move(&dest_path, &src_path);
}

int
cnode_mutate(env_t env, seL4_CPtr src, seL4_CPtr dest)
{
    cspacepath_t src_path, dest_path;

    vka_cspace_make_path(&env->vka, src, &src_path);
    vka_cspace_make_path(&env->vka, dest, &dest_path);
    return vka_cnode_mutate(&dest_path, &src_path, seL4_NilData);
}

int
cnode_recycle(env_t env, seL4_CPtr cap)
{
    cspacepath_t path;
    vka_cspace_make_path(&env->vka, cap, &path);
    return vka_cnode_recycle(&path);
}

int
cnode_revoke(env_t env, seL4_CPtr cap)
{
    cspacepath_t path;
    vka_cspace_make_path(&env->vka, cap, &path);
    return vka_cnode_revoke(&path);
}

int
cnode_rotate(env_t env, seL4_CPtr src, seL4_CPtr pivot, seL4_CPtr dest)
{
    cspacepath_t src_path, dest_path, pivot_path;

    vka_cspace_make_path(&env->vka, src, &src_path);
    vka_cspace_make_path(&env->vka, dest, &dest_path);
    vka_cspace_make_path(&env->vka, pivot, &pivot_path);
    return vka_cnode_rotate(&dest_path, seL4_NilData, &pivot_path, seL4_NilData, &src_path);
}


int
cnode_savecaller(env_t env, seL4_CPtr cap)
{
    cspacepath_t path;
    vka_cspace_make_path(&env->vka, cap, &path);
    return vka_cnode_saveCaller(&path);
}



int
are_tcbs_distinct(seL4_CPtr tcb1, seL4_CPtr tcb2)
{
    int error, i;
    seL4_UserContext regs;

    /* Initialise regs to prevent compiler warning. */
    error = seL4_TCB_ReadRegisters(tcb1, 0, 0, 1, &regs);
    if (error) {
        return error;
    }

    for (i = 0; i < 2; ++i) {
        sel4utils_set_instruction_pointer(&regs, i);
        error = seL4_TCB_WriteRegisters(tcb1, 0, 0, 1, &regs);

        /* Check that we had permission to do that and the cap was a TCB. */
        if (error) {
            return error;
        }

        error = seL4_TCB_ReadRegisters(tcb2, 0, 0, 1, &regs);

        /* Check that we had permission to do that and the cap was a TCB. */
        if (error) {
            return error;
        } else if (sel4utils_get_instruction_pointer(regs) != i) {
            return 1;
        }

    }

    return 0;
}

static void
create_args(char strings[][WORD_STRING_SIZE], char *argv[], int argc, ...)
{
    va_list args;
    va_start(args, argc);

    for (int i = 0; i < argc; i++) {
        seL4_Word arg = va_arg(args, seL4_Word);
        argv[i] = strings[i];
        snprintf(argv[i], WORD_STRING_SIZE, "%d", arg);

    }

    va_end(args);
}

void
create_helper_process(env_t env, helper_thread_t *thread)
{
    UNUSED int error;

    error = vka_alloc_endpoint(&env->vka, &thread->local_endpoint);
    assert(error == 0);

    thread->is_process = true;

    sel4utils_process_config_t config = {
        .is_elf = false,
        .create_cspace = true,
        .one_level_cspace_size_bits = env->cspace_size_bits,
        .create_vspace = true,
        .reservations = env->regions,
        .num_reservations = env->num_regions,
        .create_fault_endpoint = false,
        .fault_endpoint = { .cptr = env->endpoint },
        .priority = OUR_PRIO - 1,
#ifndef CONFIG_KERNEL_STABLE
        .asid_pool = env->asid_pool,
#endif
    };

    error = sel4utils_configure_process_custom(&thread->process, &env->vka, &env->vspace,
                                               config);
    assert(error == 0);

    /* copy the elf reservations we need into the current process */
    memcpy(thread->regions, env->regions, sizeof(sel4utils_elf_region_t) * env->num_regions);
    thread->num_regions = env->num_regions;

    /* clone data/code into vspace */
    for (int i = 0; i < env->num_regions; i++) {
        error = sel4utils_bootstrap_clone_into_vspace(&env->vspace, &thread->process.vspace, thread->regions[i].reservation);
        assert(error == 0);
    }

    thread->thread = thread->process.thread;
    assert(error == 0);
}

NORETURN static void
signal_helper_finished(seL4_CPtr local_endpoint, int val)
{
    seL4_MessageInfo_t info = seL4_MessageInfo_new(0, 0, 0, 1);
    seL4_SetMR(0, val);
    seL4_Call(local_endpoint, info);
    assert(0);
    while (1);
}

NORETURN static void
helper_thread(int argc, char **argv)
{

    helper_fn_t entry_point = (void *) atoi(argv[2]);
    seL4_CPtr local_endpoint = (seL4_CPtr) atoi(argv[3]);

    seL4_Word args[HELPER_THREAD_MAX_ARGS] = {0};
    for (int i = 4; i < argc && i - 4 < HELPER_THREAD_MAX_ARGS; i++) {
        assert(argv[i] != NULL);
        args[i - 4] = atoi(argv[i]);
    }

    /* run the thread */
    int result = entry_point(args[0], args[1], args[2], args[3]);
    signal_helper_finished(local_endpoint, result);
    /* does not return */
}

extern uintptr_t _start[];
extern uintptr_t sel4_vsyscall[];

void
start_helper(env_t env, helper_thread_t *thread, helper_fn_t entry_point,
             seL4_Word arg0, seL4_Word arg1, seL4_Word arg2, seL4_Word arg3)
{

    UNUSED int error;

    seL4_CPtr local_endpoint;

    if (thread->is_process) {
        /* copy the local endpoint */
        cspacepath_t path;
        vka_cspace_make_path(&env->vka, thread->local_endpoint.cptr, &path);
        local_endpoint = sel4utils_copy_cap_to_process(&thread->process, path);
    } else {
        local_endpoint = thread->local_endpoint.cptr;
    }

    /* If we are starting a process then the first two args are to get us
     * through the standard 'main' function and end up in helper_thread
     * if we are starting a regular thread then these will be ignored */
    create_args(thread->args_strings, thread->args, HELPER_THREAD_TOTAL_ARGS,
        0, helper_thread, (seL4_Word) entry_point, local_endpoint,
        arg0, arg1, arg2, arg3);

    if (thread->is_process) {
        thread->process.entry_point = (void*)_start;
        thread->process.sysinfo = (uintptr_t)sel4_vsyscall;
        error = sel4utils_spawn_process_v(&thread->process, &env->vka, &env->vspace,
                                        HELPER_THREAD_TOTAL_ARGS, thread->args, 1);
        assert(error == 0);
    } else {
        error = sel4utils_start_thread(&thread->thread, helper_thread,
                                       (void *) HELPER_THREAD_TOTAL_ARGS, (void *) thread->args, 1);
        assert(error == 0);
    }
}

void
cleanup_helper(env_t env, helper_thread_t *thread)
{

    vka_free_object(&env->vka, &thread->local_endpoint);

    if (thread->is_process) {
        /* free the regions (no need to unmap, as the
        * entry address space / cspace is being destroyed */
        for (int i = 0; i < thread->num_regions; i++) {
            vspace_free_reservation(&thread->process.vspace, thread->regions[i].reservation);
        }

        thread->process.fault_endpoint.cptr = 0;
        sel4utils_destroy_process(&thread->process, &env->vka);
    } else {
        sel4utils_clean_up_thread(&env->vka, &env->vspace, &thread->thread);
    }
}



void
create_helper_thread(env_t env, helper_thread_t *thread)
{
    UNUSED int error;

    error = vka_alloc_endpoint(&env->vka, &thread->local_endpoint);
    assert(error == 0);

    thread->is_process = false;
    thread->fault_endpoint = env->endpoint;
    seL4_CapData_t data = seL4_CapData_Guard_new(0, seL4_WordBits - env->cspace_size_bits);
    error = sel4utils_configure_thread(&env->vka, &env->vspace, &env->vspace, env->endpoint,
                                       OUR_PRIO - 1, env->cspace_root, data, &thread->thread);
    assert(error == 0);
}

int
wait_for_helper(helper_thread_t *thread)
{
    seL4_Word badge;

    seL4_Wait(thread->local_endpoint.cptr, &badge);
    return seL4_GetMR(0);
}

void
set_helper_priority(helper_thread_t *thread, seL4_Word prio)
{
    UNUSED int error;
    error = seL4_TCB_SetPriority(thread->thread.tcb.cptr, prio);
    assert(error == seL4_NoError);
}

void
wait_for_timer_interrupt(env_t env)
{
    seL4_Word sender_badge;
    seL4_Wait(env->timer_aep.cptr, &sender_badge);
    sel4_timer_handle_single_irq(env->timer);
}
