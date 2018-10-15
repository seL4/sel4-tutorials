/*? declare_task_ordering(['hello-world', 'hello-world-mod']) ?*/
# Hello, World!

## Getting started

1. Install [Host Dependencies](https://docs.sel4.systems/HostDependencies). It may be fastest to use the docker image
that we provide.

## Building your first program

seL4 is not an operating system. This means that it doesn't have many services out of the box. In order to run our first
program, we need to compile it as the root task---the initial userlevel program that is loaded once the kernel has booted.
At this point we can only really print to serial using a debug seL4 syscall `seL4_DebugPutCHar()` that printf is hooked up to.

All we need to do is build and run it.

/*? macros.ninja_block() ?*/

If successful, we should see the final ninja rule passing:
```
[150/150] objcopy kernel into bootable elf
$
```

The final image can be run by:
/*? macros.simulate_block() ?*/

This will run the result on a qemu instance. If everything has worked, you should see:
```
Booting all finished, dropped to user space
/*- filter TaskCompletion("hello-world", TaskContentType.ALL) -*/
Hello, World!
/*- endfilter -*/
```

## Looking at the sources

In your tutorial directory, you fill find a `CMakeLists.txt` file and a `main.c` located in `src`.

```c
#include <stdio.h>

/*- filter TaskContent("hello-world", TaskContentType.ALL) -*/

int main(int argc, char *argv[]) {
    printf("Hello, World!\n");

    return 0;
}
/*- endfilter -*/
```


### Making a change

```c
/*- filter TaskContent("hello-world-mod", TaskContentType.COMPLETED) -*/

int main(int argc, char *argv[]) {
    printf("Hello, World!\n");

    printf("Second hello\n");
    return 0;
}
/*- endfilter -*/
```
You should see something like the following if it is successful

```
/*- filter TaskCompletion("hello-world-mod", TaskContentType.COMPLETED) -*/
Second hello
/*- endfilter -*/
```

### Welcome to the 

## Troubleshooting


## Documentation

## Asking for help


/*- filter ExcludeDocs() -*/

```c
/*- filter File("src/main.c") -*/
#include <stdio.h>

/*? include_task_type_replace(["hello-world", "hello-world-mod"]) ?*/

/*- endfilter -*/
```

```cmake
/*- filter File("CMakeLists.txt") -*/
# @TAG(DATA61_BSD)

cmake_minimum_required(VERSION 3.7.2)

project(hello-world C)

add_executable(hello-world src/main.c)

target_link_libraries(hello-world
    sel4
    muslc utils sel4tutorials
    sel4muslcsys sel4platsupport sel4utils sel4debug)

DeclareRootserver(hello-world)

/*? macros.cmake_check_script(state) ?*/

/*- endfilter -*/
```
/*- endfilter -*/