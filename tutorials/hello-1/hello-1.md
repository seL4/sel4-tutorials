# seL4 Tutorial 1
/*? declare_task_ordering(['hello-world']) ?*/

This tutorial is a fairly simple
introduction to debugging, and doesn't really focus on seL4 itself, but
rather on the idea and practice of tracing errors and debugging in a
[freestanding](http://www.embedded.com/electronics-blogs/programming-pointers/4027541/Freestanding-vs-hosted-implementations)
environment.

## Learning outcomes


- Reader should become accustomed to the idea of having to debug
        not only from the perspective of their own code, but also with
        an understanding that there are lower layers beneath them that
        have a say in whether or their abstractions will work
        as intended.
- Reader should become accustomed to the idea that the compiler
        and language runtime are now part of their responsibility, and
        they should become at least trivially acquainted with the C
        runtime which they can usually ignore, and debug with it
        in mind.
- Offhandedly hints to the reader that they should become
        acquainted with the CMake build utilities.

## Walkthrough
 Currently seL4 supports two major architectures: x86
and ARM. There are platforms/boards for each architecture. For example,
pc99 is an x86 platform, while zynq7000 is an ARM platform. seL4
Tutorials can run on both architectures. In seL4 Tutorials, we only
provide one platform to represent an architecture: ia32/pc99 for x86 and
zynq7000 for ARM. The reason is that these platforms are popular and
well supported/maintained by seL4 and QEMU.

The following instructions are for ia32/pc99/x86, but should be similar
for zynq7000/ARM with just changing the config selection (../init --plat zynq7000 --tut hello-1)

First try to build the code:
```
# go to the top level directory
cd sel4-tutorials-manifest/
# create a build directory
mkdir build_hello_1
cd build_hello_1
# initialise your build directory for the first tutorial
../init --plat pc99 --tut hello-1
# build it
ninja
```
This will fail to build with the following
error:
```
[4/20] Linking C executable projects/sel4-tutorials/tutorials/hello-1/hello-1
FAILED: projects/sel4-tutorials/tutorials/hello-1/hello-1
# ...
/scratch/sel4_tutorials/exercises/build_hello_1/lib/crt1.o: In function `_start_c':
crt1.c:(.text._start_c+0x1f): undefined reference to `main'
collect2: error: ld returned 1 exit status
ninja: build stopped: subcommand failed.
```
### TASK 1 

Your task is to fix the above error. Look for TASK in `hello-1/src` within your build directory
to find the code to modify.

Regardless of the programming language used, every binary that is
created must have an entry point, which is the first instruction in the
program. In the C Runtime, this is usually `_start()`, which then calls
some other compiler-specific and platform specific functions to
initialize the program's environment, before calling the main()
function. What you see here is the linker complaining that when
`_start()` tried to call `main()`, it couldn't find a `main()` function,
because one doesn't exist. Create a `main()` function, and proceed to the
next step in the slides.

```c
/*- filter TaskContent("hello-world", TaskContentType.BEFORE, completion="Booting all finished, dropped to user space") -*/
/* TASK 1: add a main function to print a message */
/* hint: printf */

int main(void) {

    return 0;
}
/*- endfilter -*/
```


```c
/*- filter TaskContent("hello-world", TaskContentType.COMPLETED, completion="hello world") -*/
/* TASK 1: add a main function to print a message */
/* hint: printf */

int main(void) {
    printf("hello world\n");

    return 0;
}
/*- endfilter -*/
```

This next step is meant to show the user some basics for how to go about
debugging in a freestanding environment. Most of the time you'll have to
manually step through code, often without the help of a debugger, so
this step forces a Divide by zero exception and tries to show you how
debugging an exception looks. In the real world, in this kind of
scenario, you'd usually want to know the instruction that triggered the
exception, then find out what that instruction does that is wrong, and
fix it.

Once you have fixed the problem, the build should succeed and you can
run the example as follows:

```
cd build_hello_1
ninja
./simulate
```

If you've succeeded, qemu should output:

```
Starting node #0 with APIC ID 0 Booting all finished, dropped to user space
hello world
```
/*- filter ExcludeDocs() -*/
## Sources

```c
/*- filter File("src/main.c") -*/
/*
 * @TAG(DATA61_BSD)
 */

/*
 * seL4 tutorial part 1:  simple printf
 */


#include <stdio.h>


#include <utils/zf_log.h>
#include <sel4utils/sel4_zf_logif.h>

/*? include_task("hello-world") ?*/
/*- endfilter -*/
```


```cmake
/*- filter File("CMakeLists.txt") -*/
# @TAG(DATA61_BSD)

cmake_minimum_required(VERSION 3.7.2)

project(hello-1 C)

add_executable(hello-1 src/main.c)
message("test")
target_link_libraries(hello-1
    sel4
    muslc utils sel4tutorials
    sel4muslcsys sel4platsupport sel4utils sel4debug)

DeclareRootserver(hello-1)

/*? macros.cmake_check_script(state) ?*/

/*- endfilter -*/
```
/*- endfilter -*/