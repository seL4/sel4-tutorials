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

/*
 * seL4 tutorial part 4: application to be run in a process
 */

#include <stdio.h>
#include <assert.h>

#include <sel4/sel4.h>
#include <sel4utils/process.h>

/* constants */
#define EP_CPTR SEL4UTILS_FIRST_FREE // where the cap for the endpoint was placed.
#define MSG_DATA 0x2 //  arbitrary data to send

int main(int argc, char **argv) {
    seL4_MessageInfo_t tag;
    seL4_Word msg;

    printf("timer client: hey hey hey\n");

    /*
     * send a message to our parent, and wait for a reply
     */

    /* set the data to send. We send it in the first message register */
    tag = seL4_MessageInfo_new(0, 0, 0, 1);
    seL4_SetMR(0, MSG_DATA);

    /* send and wait for a reply */
    tag = seL4_Call(EP_CPTR, tag);

    /* check that we got the expected repy */
    assert(seL4_MessageInfo_get_length(tag) == 1);
    msg = seL4_GetMR(0);

    printf("timer client wakes up: got the current timer tick: %zu\n", msg);

    return 0;
}

