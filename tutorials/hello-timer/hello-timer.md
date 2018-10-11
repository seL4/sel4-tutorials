---
toc: true
---

# seL4 Timer tutorial


This tutorial demonstrates how to set up and use a basic timer driver.

You'll observe that the things you've already covered in the other
tutorials are already filled out and you don't have to repeat them: in
much the same way, we won't be repeating conceptual explanations on this
page, if they were covered by a previous tutorial in the series.

## Learning outcomes


- Allocate a notification object.
- Set up a timer provided by seL4 libs.
- Use `seL4_libs` functions to manipulate timer and
      handle interrupts.

## Walkthrough


First, build and run the tutorial:
```
# create a build directory
mkdir build_hello_timer
cd build_hello_timer
# initialise your build directory
../init --plat pc99 --tut hello-timer
# build it
ninja
# run it in qemu
./simulate
```

Before you have done any tasks, when running the tutorial should show
the timer and timer client waking up, then a cap fault is triggered due
to incomplete code:
```
main: hello world
timer client: hey hey hey
main: got a message from 0x61 to sleep 2 seconds
Caught cap fault in send phase at address 0x0
while trying to handle:
vm fault on data at address 0x0 with status 0x4
in thread 0xffaf7900 "hello-timer" at address 0x804841a
With stack:
0x80bc880: 0x806838c
0x80bc884: 0x61
0x80bc888: 0x2
0x80bc88c: 0x80482ff
0x80bc890: 0x20
0x80bc894: 0x0
0x80bc898: 0x0
0x80bc89c: 0x0
0x80bc8a0: 0x80bc8c0
0x80bc8a4: 0x80bc8d0
0x80bc8a8: 0x80be010
0x80bc8ac: 0x7
0x80bc8b0: 0x0
0x80bc8b4: 0x0
0x80bc8b8: 0x0
0x80bc8bc: 0x10000000
QEMU: Terminated
```

Look for `TASK` in the `apps/hello-timer` and `apps/hello-timer` directory for
each task.

### TASK 1


The first task is to allocate a notification object to receive
interrupts on. The output will not change as a result of completing this
task.

### TASK 2


Use our library function `sel4platsupport_get_default_timer` to
initialise a timer driver. Assign it to the `timer` global variable.

After this change, the server will no longer fault and the output will
look something like this:
```
Searching for ACPI_SIG_RSDP
Parsing ACPI tables
timer client: hey hey hey
hpet_get_timer@hpet.c:362 Requested fsb delivery, but timer0 does not support
main: hello world
main: got a message from 0x61 to sleep 2 seconds
timer client wakes up: got the current timer tick: 17299121
```

### TASK 3


While at the end of the previous task the tutorial appears to be
working, the main thread replies immediately to the client and doesn't
wait at all.

Consequently, the final task is to interact with the timer: set a
timeout, wait for an interrupt on the created notification, and handle
it.

After this task is completed you should see a 2 second wait between the
message being sent and received.