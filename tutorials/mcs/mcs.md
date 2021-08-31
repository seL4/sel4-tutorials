<!--
  Copyright 2017, Data61, CSIRO (ABN 41 687 119 230)

  SPDX-License-Identifier: BSD-2-Clause
-->

# MCS Extensions
/*? declare_task_ordering(['mcs-start','mcs-periodic', 'mcs-unbind', 'mcs-bind', 'mcs-sporadic', 
'mcs-server','mcs-badge','mcs-fault']) ?*/

## Prerequisites

1. [Set up your machine](https://docs.sel4.systems/HostDependencies).
2. You should be familiar with the seL4 Api. Otherwise do the [Mechanisms tutorials](https://docs.sel4.systems/Tutorials)


## Initialising

Then initialise the tutorial:

/*? macros.tutorial_init("mcs") ?*/

## Outcomes

This tutorial presents the features in the upcoming MCS extensions for seL4, which are currently undergoing
verification. For further context on the new features, please see the 
[paper](https://trustworthy.systems/publications/csiro_full_text/Lyons_MAH_18.pdf) or [phd](https://github.com/pingerino/phd/blob/master/phd.pdf)
 which provides a comprehensive background on the changes.

1. Learn about the MCS new kernel API.
1. Be able to create and configure scheduling contexts.
2. Learn the jargon *passive server*.
3. Spawn round-robin and periodic threads.

## Background

The MCS extensions provide capability-based access to CPU time, and provide mechanisms to limit the upper
bound of execution of a thread. 

### Scheduling Contexts

Scheduling contexts are a new object type in the kernel, which contain scheduling parameters amoung other 
accounting details. Most importantly, scheduling contexts contain a *budget* and a *period*, which 
represent an upper bound on execution time allocated: the kernel will enforce that threads cannot execute
for more than *budget* microseconds out of *period* microseconds. 

### SchedControl

Parameters for scheduling contexts are configured by invoking `seL4_SchedControl` capabilities, one of 
which is provided per CPU. The invoked `seL4_SchedControl` determines which processing core that specific 
scheduling context provides access to.

Scheduling contexts can be configured as *full* or *partial*. Full scheduling contexts have `budget == 
period` and grant access to 100% of CPU time. Partial scheduling contexts grant access to an upper bound of
 `budget/period` CPU time.

The code example below configures a 
scheduling context with a budget and period both equal to 1000us. Because the budget and period are equal, 
the scheduling context is treated as round-robin 

```c
/*-- filter TaskContent("mcs-start", TaskContentType.ALL, subtask='configure') -*/
    error = seL4_SchedControl_Configure(sched_control, sched_context, US_IN_S, US_IN_S, 0, 0);
    ZF_LOGF_IF(error != seL4_NoError, "Failed to configure schedcontext");
/*-- endfilter -*/
```

### SchedContext Binding

Thread control blocks (TCBs) must have a scheduling context configured with non-zero budget and period
 in order to be picked by the scheduler. This 
can by invoking the scheduling context capability with the `seL4_SchedContext_Bind` invocation, or by
using `seL4_TCB_SetSchedParams`, which takes a scheduling context capability. Below is example code for
binding a TCB and a scheduling context.

```c
/*-- filter TaskContent("mcs-start", TaskContentType.ALL, subtask='bind_yield') -*/
    error = seL4_SchedContext_Bind(sched_context, spinner_tcb);
    ZF_LOGF_IF(error != seL4_NoError, "Failed to bind sched_context to round_robin_tcb");
/*-- endfilter -*/
```

TCB's can only be bound to one scheduling context at a time, and vice versa. If a scheduling context has not
been configured with any time, then although the TCB has a scheduling context it will remain ineligible 
for scheduling. 

### Bounding execution

For partial scheduling contexts, an upper bound on execution is enforced by seL4 using the *sporadic server*
algorithm, which work by guaranteeing the *sliding window* constrain, meaning that during any period, the
budget cannot be exceeded. This is achieved by tracking the eligible budget in chunks called 
*replenishments* (abbreviated to `refills` in the API for brevity). A replenishment is simply an amount 
 of time, and a timestamp from which that time can be consumed. We explain this now through an example:

Consider a scheduling context with period *T* and budget *C*. Initially, the scheduling context has 
a single replenishment of size *C* which is eligible to be used from time *t*. 

The scheduling context is scheduled at time *t* and blocks at time *t + n*. A new replenishment is then scheduled
for time *t+T* for *n*. The existing replenishment is updated to *C - n*, subtracting the amount consumed.
For further details on the sporadic server algorithm, see the 
[original paper](https://dl.acm.org/citation.cfm?id=917665).

If the replenishment data structure is full, replenishments are merged and the upper bound on execution is
reduced. For this reason, the bound on execution is configurable with the `extra_refills` parameter
on scheduling contexts. By default, scheduling contexts contain only two parameters, meaning if a 
scheduling context is blocked, switched or preempted more than twice, the rest of the budget is forfeit until 
the next period. `extra_refills` provides more replenishment data structures in a scheduling context. Note 
that the higher the number of replenishments the more fragmentation of budget can occur, which will increase
scheduling overhead.

`extra_refills` itself is bounded by the size of a scheduling context, which is itself configurable. 
On scheduling context creation a size can be specified, and must be `> seL4_MinSchedContextBits`. The 
maximum number of extra refills that can fit into a specific scheduling context size can be calculated
with the function `seL4_MaxExtraRefills()` provided in `libsel4`.

Threads bound to scheduling contexts that do not have an available replenishment are placed into an ordered
queue of threads, and woken once their next replenishment is ready.

### Scheduler

The seL4 scheduler is largely unchanged: the highest priority, non-blocked thread with a configured
 scheduling context that has available budget is chosen by the scheduler to run.

### Passive servers

The MCS extensions allow for RPC style servers to run on client TCBs' scheduling contexts. This is achived by 
unbinding the scheduling context once a server is blocked on an endpoint, rendering the server *passive*. 
Caller scheduling contexts are donated to the server on `seL4_Call` and returned on `seL4_ReplyRecv`.

Passive servers can also receive scheduling contexts from their bound notification object, which is 
achieved by binding a notification object using `seL4_SchedContext_Bind`.

### Timeout faults

Threads can register a timeout fault handler using `seL4_TCB_SetTimeoutEndpoint`. Timeout fault
handlers are optional and are raised when a thread's replenishment expires *and* they have a valid handler
registered. The timeout fault message from the kernel contains the data word which can be used to identify the
scheduling context that the thread was using when the timeout fault occurred, and the amount of time
consumed by the thread since the last fault or `seL4_SchedContext_Consumed`. 

### New invocations

* `seL4_SchedContext_Bind` - bind a TCB or Notification capability to the invoked scheduling context.
* `seL4_SchedContext_Unbind` - unbind any objects from the invoked scheduling context.
* `seL4_SchedContext_UnbindObject`- unbind a specific object from the invoked scheduling context.
* `seL4_SchedContext_YieldTo` - if the thread running on the invoked scheduling context is 
schedulable, place it at the head of the scheduling queue for its priority. For same priority threads, this 
will result in the target thread being scheduled. Return the amount of time consumed by this scheduling 
context since the last timeout fault, `YieldTo` or `Consumed` invocation.
* `seL4_SchedContext_Consumed` - Return the amount of time consumed by this scheduling 
context since the last timeout fault, `YieldTo` or `Consumed` invocation.
* `seL4_TCB_SetTimeoutEndpoint` - Set the timeout fault endpoint for a TCB.

### Reply objects

The MCS API also makes the reply channel explicit: reply capabilities are now fully fledged objects 
which must be provided by a thread on `seL4_Recv` operations. They are used to track the scheduling
context donation chain and return donated scheduling contexts to callers.

Please see the [release notes](https://docs.sel4.systems/sel4_release/seL4_9.0.0-mcs) and 
[manual](https://docs.sel4.systems/sel4_release/seL4_9.0.0-mcs.html) for further details
 on the API changes. 
 
## Exercises

In the initial state of the tutorial, the main function in `mcs.c` is running in one process, and the 
following loop from `spinner.c` is running in another process:


```
/*-- filter TaskContent("mcs-start", TaskContentType.ALL, subtask='yield') -*/
    int i = 0;
    while (1) {
        printf("Yield\n");
        seL4_Yield();
    }
/*-- endfilter -*/
```

Both processes share the same priority. The code in `mcs.c` binds
a scheduling context (`sched_context`) to the TCB of the spinner process (`spinner_tcb)` with round-robin scheduling parameters. As a result, you should see
 something like the following output, which continues uninterrupted:

```
/*-- filter TaskCompletion("mcs-start", TaskContentType.ALL) -*/
Yield
Yield
Yield
/*-- endfilter -*/
```

### Periodic threads

**Exercise** Reconfigure `sched_context` with the following periodic scheduling parameters,
 (budget = `0.9 * US_IN_S`, period = `1 * US_IN_S`).

 ```c
/*-- filter TaskContent("mcs-start", TaskContentType.ALL, subtask='periodic') -*/
    //TODO reconfigure sched_context to be periodic
/*-- endfilter -*/
/*-- filter ExcludeDocs() -*/
/*-- filter TaskContent("mcs-periodic", TaskContentType.COMPLETED, subtask='periodic') -*/
    // reconfigure sched_context to be periodic
    error = seL4_SchedControl_Configure(sched_control, sched_context, 0.9 * US_IN_S, 1 * US_IN_S, 0, 0);
    ZF_LOGF_IF(error != seL4_NoError, "Failed to configure schedcontext");
/*-- endfilter -*/
/*-- endfilter -*/
```

By completing this task successfully, the output will not change, but the rate that the output is
printed will slow: each subsequent line should be output once the period has elapsed. You should now
be able to see the loop where the `mcs.c` process and `spinner.c` process alternate, until the `mcs.c` 
process blocks, at which point `"Yield"` is emitted by `spinner.c` every second, as shown below:

```
/*-- filter TaskCompletion("mcs-periodic", TaskContentType.ALL) -*/
Yield
Tick 0
Yield
Tick 1
Yield
Tick 2
Yield
Tick 3
Yield
Tick 4
Yield
Tick 5
Yield
Tick 6
Yield
Tick 7
Yield
Tick 8
Yield
Yield
Yield
/*-- endfilter -*/
```
Before you completed this task, the scheduling context was round-robin, and so was 
schedulable immediately after the call to `seL4_Yield`. 
By changing
the scheduling parameters of `sched_context` to periodic parameters (budget < period), each time 
`seL4_Yield()` is called the available budget in the scheduling context is abandoned, causing the 
thread to sleep until the next replenishment, determined by the period.  

### Unbinding scheduling contexts

You can cease a threads execution by unbinding the scheduling context.
Unlike *suspending* a thread via `seL4_TCB_Suspend`, unbinding will not change the thread state. Using suspend
cancels any system calls in process (e.g IPC) and renders the thread unschedulable by changing the
thread state. Unbinding a scheduling context does not alter the thread state, but merely removes the thread
from the scheduler queues.

**Exercise** Unbind `sched_context` to stop the spinner process from running.
```c
/*-- filter TaskContent("mcs-start", TaskContentType.ALL, subtask='unbind') -*/
    //TODO unbind sched_context to stop yielding thread 
/*- endfilter --*/
/*-- filter ExcludeDocs() --*/
/*-- filter TaskContent("mcs-unbind", TaskContentType.COMPLETED, subtask='unbind') -*/
    // unbind sched_context to stop the yielding thread
    error = seL4_SchedContext_Unbind(sched_context);
    ZF_LOGF_IF(error, "Failed to unbind sched_context");
/*-- endfilter -*/
/*-- filter TaskCompletion("mcs-unbind", TaskContentType.COMPLETED) -*/
Tick 6
Yield
Tick 7
Yield
Tick 8

/*-- endfilter --*/
/*-- endfilter --*/
```

On success, you should see the output from the yielding thread stop.

### Sporadic threads

Your next task is to use a different process, `sender` to experiment with sporadic tasks. The 
`sender` process is ready to run, and just needs a scheduling context in order to do so.

**Exercise** First, bind `sched_context` to `sender_tcb`.

 ```c
/*-- filter TaskContent("mcs-start", TaskContentType.ALL, subtask='bind') -*/
    //TODO bind sched_context to sender_tcb
/*-- endfilter -*/
/*-- filter ExcludeDocs() -*/
/*-- filter TaskContent("mcs-bind", TaskContentType.COMPLETED, subtask='bind') -*/
    // bind sched_context to sender_tcb
    error = seL4_SchedContext_Bind(sched_context, sender_tcb);
    ZF_LOGF_IF(error != seL4_NoError, "Failed to bind schedcontext");
/*-- endfilter -*/
/*-- endfilter -*/
```

The output should look like the following:
```
...
/*-- filter TaskCompletion("mcs-bind", TaskContentType.COMPLETED) -*/
Tock 3
Tock 4
Tock 5
Tock 6
/*-- endfilter -*/
```

Note the rate of the output: currently, you should see 2 lines come out at a time, with roughly  
a second break between (the period of the scheduling context you set earlier). This is because
scheduling context only has the minimum sporadic refills (see background), and each time a context switch 
occurs a refill is used up to schedule another. 

**Exercise** Reconfigure the `sched_context` to an extra 6 refills, such that all of the `Tock` output 
occurs in one go.

 ```c
/*-- filter TaskContent("mcs-start", TaskContentType.ALL, subtask='sporadic') -*/
    //TODO reconfigure sched_context to be periodic with 6 extra refills
/*-- endfilter -*/
/*-- filter ExcludeDocs() -*/
/*-- filter TaskContent("mcs-sporadic", TaskContentType.COMPLETED, subtask='sporadic') -*/
    // reconfigure sched_context to be periodic with 6 extra refills
    error = seL4_SchedControl_Configure(sched_control, sched_context, 0.9 * US_IN_S, 1 * US_IN_S, 6, 0);
    ZF_LOGF_IF(error != seL4_NoError, "Failed to configure schedcontext");
/*-- endfilter -*/
/*-- filter TaskCompletion("mcs-sporadic", TaskContentType.ALL) -*/
Tock 4
Tock 5
Tock 6
Tock 7
/*-- endfilter -*/
/*-- endfilter -*/
```

### Passive servers

Now look to the third process, `server.c`, which is a very basic echo server. It currently does 
not have a scheduling context, and needs one to initialise.

**Exercise** Bind `sched_context` to `server_tcb`. 

 ```c
/*-- filter TaskContent("mcs-start", TaskContentType.ALL, subtask='server') -*/
    //TODO bind sched_context to server_tcb
/*-- endfilter -*/
/*-- filter ExcludeDocs() -*/
/*-- filter TaskContent("mcs-server", TaskContentType.COMPLETED, subtask='server') -*/
    // bind the servers sched context
    error = seL4_SchedContext_Bind(sched_context, server_tcb);
    ZF_LOGF_IF(error != seL4_NoError, "Failed to bind sched_context to server_tcb");
/*-- endfilter -*/
/*-- endfilter -*/
```

Now you should see the server initialise and echo the messages sent. Note the initialisation protocol:
first, you bound `sched_context` to the server. At this point, in `server.c`, the server calls 
`seL4_NBSendRecv` which sends an IPC message on `endpoint`, indicating that the server is now initialised.
The output should be as follows

```
/*-- filter TaskCompletion("mcs-server", TaskContentType.COMPLETED) -*/
Tock 8
Starting server
Wait for server
Server initialising
running
passive
echo server
Yield
/*-- endfilter -*/
```

The following code then converts the server to passive:

```c 
/*-- filter TaskContent("mcs-start", TaskContentType.ALL, subtask='passive') -*/
    // convert to passive
    error = seL4_SchedContext_Unbind(sched_context);
/*-- endfilter -*/
```

From this point, the server runs on the `mcs` process's scheduling context.

### Timeout Faults

But before we discuss timeout faults, we must first discuss the differences
between configuring a fault endpoint on the master vs the MCS kernel. There is a
minor difference in the way that the kernel is informed of the cap to a fault
endpoint, between the master and MCS kernels.

Regardless though, on both versions of the kernel, to inform the kernel of the
fault endpoint for a thread, call the usual `seL4_TCB_SetSpace()`.

#### Configuring a fault endpoint on the MCS kernel:

On the MCS kernel the cap given to the kernel must be a cap to an object in
the CSpace of the thread which is *calling the syscall* (`seL4_TCB_Configure()`)
to give the cap to the kernel.

This calling thread may be the handler thread, or some other thread which
manages both the handler thread and the faulting thread. Or in an oddly designed
system, it might be the faulting thread itself if the faulting thread is allowed
to configure its own fault endpoint.

The reason for this difference is merely that it is faster to lookup the fault
endpoint this way since it is looked up only once at the time it is configured.

#### Configuring a fault endpoint on the Master kernel:

On the Master kernel the cap given to the kernel must be a cap to an object in
the CSpace of the *faulting thread*.

On the Master kernel, the fault endpoint cap is looked up from within the CSpace
of the faulting thread everytime a fault occurs.

#### Exercise:

**Exercise** Set the data field of `sched_context` using `seL4_SchedControl_Configure` and set a 10s period, 1ms 
budget and 0 extra refills.

```c
/*-- filter TaskContent("mcs-start", TaskContentType.ALL, subtask='badge') -*/
    //TODO reconfigure sched_context with 10s period, 1ms budget, 0 extra refills and data of 5.
/*-- endfilter -*/
/*-- filter ExcludeDocs() -*/
/*-- filter TaskContent("mcs-badge", TaskContentType.COMPLETED, subtask='badge') -*/

    // reconfigure sched_context with 1s period, 500 microsecond budget, 0 extra refills and data of 5.
    error = seL4_SchedControl_Configure(sched_control, sched_context, 500, US_IN_S, 0, 5);
    ZF_LOGF_IF(error != seL4_NoError, "Failed to configure sched_context");
/*-- endfilter -*/
/*-- filter TaskCompletion("mcs-badge", TaskContentType.COMPLETED) -*/
Tock 8
Starting server
Wait for server
Server initialising
running
passive
echo server
/*-- endfilter -*/
/*-- endfilter -*/
```
The code then binds the scheduling context back to `spinner_tcb`, which starts yielding again.

**Exercise** set the timeout fault endpoint for `spinner_tcb`. 


```c
/*-- filter TaskContent("mcs-start", TaskContentType.ALL, subtask='fault') -*/
    //TODO set endpoint as the timeout fault handler for spinner_tcb
/*-- endfilter -*/
/*-- filter ExcludeDocs() -*/
/*-- filter TaskContent("mcs-fault", TaskContentType.COMPLETED, subtask='fault') -*/
    // set endpoint as the timeout fault handler for spinner_tcb
    error = seL4_TCB_SetTimeoutEndpoint(spinner_tcb, endpoint);
    ZF_LOGF_IF(error != seL4_NoError, "Failed to set timeout fault endpoint for spinner");
/*-- endfilter -*/
/*-- endfilter -*/
```

When the `spinner` faults, you should see the following output:

```
/*-- filter TaskCompletion("mcs-fault", TaskContentType.COMPLETED) -*/
Received timeout fault
Success!
/*-- endfilter -*/
```

### Further exercises

That's all for the detailed content of this tutorial. Below we list other ideas for exercises you can try,
to become more familiar with the MCS extensions.

* Set up a passive server with a timeout fault handlers, with policies for clients that exhaust their budget.
* Experiment with notification binding on a passive server, by binding both a notification object to the 
server TCB and an SC to the notification object.

/*? macros.help_block() ?*/

/*-- filter ExcludeDocs() -*/
```c
/*-- filter ELF("spinner", passive=True) -*/
/*- set _ = state.stash.start_elf("spinner") -*/
#include <stdio.h>
#include <sel4/sel4.h>
#include <utils/util.h>

int main(int c, char *argv[]) {
/*? include_task_type_append([("mcs-start", 'yield')]) ?*/
    return 0;
}
/*-- endfilter -*/
/*-- filter ELF("sender", passive=True) -*/
/*- set _ = state.stash.start_elf("sender") -*/
#include <stdio.h>
#include <sel4/sel4.h>

/*? capdl_alloc_cap(seL4_EndpointObject, "endpoint", "endpoint", write=True, read=True, grant=True) ?*/
int main(int c, char *argv[]) {
    int i = 0;
    while (1) {
        seL4_MessageInfo_t info = seL4_MessageInfo_new(0, 0, 0, 1);
        seL4_SetMR(0, i);
        seL4_Send(endpoint, info);
        i++;
    }
    return 0;
}
/*-- endfilter -*/
/*-- filter ELF("server", passive=True) -*/
/*- set _ = state.stash.start_elf("server") -*/
#include <stdio.h>
#include <sel4/sel4.h>

/*? capdl_alloc_cap(seL4_EndpointObject, "endpoint", "endpoint", write=True, read=True, grant=True) ?*/
/*? capdl_alloc_cap(seL4_RTReplyObject, "reply", "reply", read=True, write=True, grant=True) ?*/

int main(int c, char *argv[]) {

    printf("Server initialising\n");
    seL4_MessageInfo_t info = seL4_NBSendRecv(endpoint, info, endpoint, NULL, reply);
    while (1) {
        int i = 0;
        for (; i < seL4_MsgMaxLength && i < seL4_MessageInfo_get_length(info); i++) {
            seL4_DebugPutChar(seL4_GetMR(i));
        }
        seL4_DebugPutChar('\n');
        seL4_SetMR(0, i);
        info = seL4_ReplyRecv(endpoint, seL4_MessageInfo_new(0, 0, 0, i), NULL, reply);
    }

    return 0;
}
/*-- endfilter -*/
/*-- filter ELF("mcs") -*/
/*- set _ = state.stash.start_elf("mcs") -*/
#include <assert.h>
#include <sel4/sel4.h>
#include <stdio.h>
#include <string.h>
#include <utils/util.h>

// CSlots pre-initialised in this CSpace
// capability to a scheduling context
/*? capdl_alloc_cap(seL4_SchedContextObject, "sched_context", "sched_context") ?*/
/*# Small hack to set the size of the sc object #*/
/*- set sc = capdl_alloc_obj(seL4_SchedContextObject, "sched_context") -*/
/*- set _ = sc.__setattr__("size_bits", 8) -*/
// the seL4_SchedControl capabilty for the current core
/*? capdl_sched_control("sched_control") ?*/
// capability to the tcb of the server process
/*? capdl_elf_tcb("server", "server_tcb") ?*/
// capability to the tcb of the spinner process
/*? capdl_elf_tcb("spinner", "spinner_tcb") ?*/
// capability to the tcb of the sender process
/*? capdl_elf_tcb("sender", "sender_tcb") ?*/
// capability to an endpoint, shared with 'sender' and 'server' 
/*? capdl_alloc_cap(seL4_EndpointObject, "endpoint", "endpoint", write=True, read=True, grant=True) ?*/
// capability to a reply object
/*? capdl_alloc_cap(seL4_RTReplyObject, "reply2", "reply", write=True, read=True, grant=True) ?*/

int main(int c, char *argv[]) {
    seL4_Error error;

    // configure sc
    /*? include_task_type_append([("mcs-start", 'configure')]) ?*/
    // bind it to `spinner_tcb`
    /*? include_task_type_append([("mcs-start", 'bind_yield')]) ?*/

    int i = 0; 
    for (; i < 9; i++) {
        seL4_Yield();
        printf("Tick %d\n", i);
    }

    /*? include_task_type_replace([("mcs-start", 'periodic'), ("mcs-periodic", 'periodic')]) ?*/
    /*? include_task_type_replace([("mcs-start", 'unbind'), ("mcs-unbind", 'unbind')]) ?*/
    /*? include_task_type_replace([("mcs-start", 'bind'), ("mcs-bind", 'bind')]) ?*/
    /*? include_task_type_replace([("mcs-start", 'sporadic'), ("mcs-sporadic", 'sporadic')]) ?*/
    for (int i = 0; i < 9; i++) {
        seL4_Wait(endpoint, NULL);
        printf("Tock %d\n", (int) seL4_GetMR(0));
    }
 
    
    error = seL4_SchedContext_UnbindObject(sched_context, sender_tcb);
    ZF_LOGF_IF(error, "Failed to unbind sched_context from sender_tcb");
    
    /* suspend the sender to get them off endpoint */
    error = seL4_TCB_Suspend(sender_tcb);
    ZF_LOGF_IF(error, "Failed to suspend sender_tcb");

    error = seL4_TCB_SetPriority(server_tcb, server_tcb, 253);
    ZF_LOGF_IF(error, "Failed to decrease server's priority");
    printf("Starting server\n");
    /*? include_task_type_replace([("mcs-start", 'server'), ("mcs-server", 'server')]) ?*/
    // wait for it to initialise
    printf("Wait for server\n"); 
    seL4_Wait(endpoint, NULL);
   
    /*? include_task_type_append([("mcs-start", 'passive')]) ?*/
    ZF_LOGF_IF(error != seL4_NoError, "Failed to unbind sched context");

    const char *messages[] = {
        "running", 
        "passive",
        "echo server",
        NULL,
    };
 
    for (int i = 0; messages[i] != NULL; i++) {
        int m = 0;
        for (; m < strlen(messages[i]) && m < seL4_MsgMaxLength; m++) {
            seL4_SetMR(m, messages[i][m]);
        }
        seL4_Call(endpoint, seL4_MessageInfo_new(0, 0, 0, m));
    }

    /*? include_task_type_replace([("mcs-start", 'badge'), ("mcs-badge", 'badge')]) ?*/
    /*? include_task_type_replace([("mcs-start", 'fault'), ("mcs-fault", 'fault')]) ?*/
    
    error = seL4_SchedContext_Bind(sched_context, spinner_tcb);
    ZF_LOGF_IF(error, "Failed to bind sched_context to spinner_tcb");

    seL4_MessageInfo_t info = seL4_Recv(endpoint, NULL, reply);
    /* parse the fault info from the message */
    seL4_Fault_t fault = seL4_getArchFault(info);
    ZF_LOGF_IF(seL4_Fault_get_seL4_FaultType(fault) != seL4_Fault_Timeout, "Not a timeout fault");
    printf("Received timeout fault\n");
    ZF_LOGF_IF(seL4_Fault_Timeout_get_data(fault) != 5, "Incorrect data");
    
    printf("Success!\n");

    return 0;
}
/*-- endfilter -*/
```
/*? ExternalFile("CMakeLists.txt") ?*/
/*- endfilter -*/
