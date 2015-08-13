/*
 * Copyright 2015, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

/*
 * seL4 tutorial part 4: application to be run in a process
 */

#include <stdio.h>
#include <assert.h>

#include <sel4/sel4.h>

/* constants */
#define EP_CPTR 0x3 // where the cap for the endpoint was placed.
#define EP_BADGE 0x61 // arbitrary (but unique) number for a badge
#define MSG_DATA 0x6161 //  arbitrary data to send

int main(int argc, char **argv) {
    seL4_MessageInfo_t tag;
    seL4_Word msg;

    printf("process_2: hey hey hey\n");

    /*
     * send a message to our parent, and wait for a reply
     */

    /* TODO: set the data to send. We send it in the first message register */
    tag = seL4_MessageInfo_new(0, 0, 0, 1);
    seL4_SetMR(0, MSG_DATA);

    /* TODO: send and wait for a reply: hint seL4_Call() */

    /* check that we got the expected repy */
    assert(seL4_MessageInfo_get_length(tag) == 1);
    msg = seL4_GetMR(0);
    assert(msg == ~MSG_DATA);

    printf("process_2: got a reply: %#x\n", msg);

    return 0;
}

