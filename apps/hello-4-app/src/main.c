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
#include <sel4utils/process.h>

#include <utils/zf_log.h>
#include <sel4utils/sel4_zf_logif.h>

/* constants */
#define EP_CPTR SEL4UTILS_FIRST_FREE // where the cap for the endpoint was placed.
#define MSG_DATA 0x6161 //  arbitrary data to send

int main(int argc, char **argv) {
    seL4_MessageInfo_t tag;
    seL4_Word msg;

    printf("process_2: hey hey hey\n");

    /*
     * send a message to our parent, and wait for a reply
     */

    /* set the data to send. We send it in the first message register */
    tag = seL4_MessageInfo_new(0, 0, 0, 1);
    seL4_SetMR(0, MSG_DATA);

    /* TODO 9: send and wait for a reply */
    /* hint 1: seL4_Call()
     * seL4_MessageInfo_t seL4_Call(seL4_CPtr dest, seL4_MessageInfo_t msgInfo)
     * @param dest The capability to be invoked.
     * @param msgInfo The messageinfo structure for the IPC.  This specifies information about the message to send (such as the number of message registers to send).
     * @return A seL4_MessageInfo_t structure.  This is information about the repy message.
     * Link to source: https://wiki.sel4.systems/seL4%20Tutorial%204#TODO_9:
     * You can find out more about it in the API manual: http://sel4.systems/Info/Docs/seL4-manual-3.0.0.pdf
     *
     * hint 2: the endpoint cap is in slot EP_CPTR
     */

    /* check that we got the expected repy */
    ZF_LOGF_IF(seL4_MessageInfo_get_length(tag) != 1,
        "Length of the data send from root thread was not what was expected.\n"
        "\tHow many registers did you set with seL4_SetMR, within the root thread?\n");
    msg = seL4_GetMR(0);
    ZF_LOGF_IF(msg != ~MSG_DATA,
        "Unexpected response from root thread.\n");

    printf("process_2: got a reply: %#x\n", msg);

    return 0;
}
