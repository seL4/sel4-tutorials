<!--
  Copyright 2017, Data61, CSIRO (ABN 41 687 119 230)

  SPDX-License-Identifier: BSD-2-Clause
-->

/*? declare_task_ordering([
    'crossvm'
]) ?*/

# CAmkES VM: Cross VM Connectors

This tutorial provides an introduction to using the cross virtual machine (VM) connector mechanisms
provided by seL4 and Camkes in order to connect processes in a guest Linux instance to Camkes components.

## Prerequisites

1. [Set up your machine](https://docs.sel4.systems/HostDependencies#camkes-build-dependencies).
1. [Camkes VM](https://docs.sel4.systems/Tutorials/camkes-vm-linux)

## Outcomes

By the end of this tutorial, you should be able to:

* Configure processes in a Linux guest VM to communicate with CAmkES components

## Background

In order to connect guest Linux instances to CAmkES components,
three additional kernel modules must be installed in the guest.
These modules are included in the root filesystem by default:

- `dataport`: facilitates setting up shared memory between the guest
and CAmkES components.
- `consumes_event`: allows a process in the guest to wait or poll
for an event sent by a CAmkES component.
- `emits_event`: allows a process to emit an event to a CAmkES component.

Each type of module can be statically assigned to one or more file descriptors to associate that
file descriptor with a specific instance of an interface. `ioctl` can then be used to
manipulate that file descriptor and use the module.

### Dataports

Dataports are the mechanism which allows guests and components to share memory.
The dataport initialisation process is as follows:

- The guest process uses `ioctl` on on the file associated with the dataport and specify a
 page-aligned size for the shared memory.
- The dataport kernel module in the guest then allocates a page-aligned buffer of the requested size,
  and makes a hypercall to the VMM, with the guest physical address and id of the data port.
  The ID is derived from the file on which `ioctl` was called.
- The virtual machine manager (VMM) then modifies the guest's address space, creating the shared memory region.
 between a camkes component and the guest.
- Linux processes can then map this memory into their own address space by calling `mmap` on the file
  associated with the dataport.

### Emitting Events

Guest processes can emit events by using `ioctl` on files associated with the event interface.
This results in the `emits_event` kernel module in the guest making a
making a hypercall to the VMM, which triggers the event and resumes the guest.

### Consuming Events

Linux process can wait or poll for an event by calling `poll`
on the file associated with that event, using the timeout argument to
specify whether or not it should block. The event it polls for is
`POLLIN`. When the VMM receives an event destined for the guest, it places
the event id in some memory shared between the VMM and the
`consumes_event` kernel module, and then injects an interrupt into the
guest. The `consumes_event` kernel module is registered to handle this
interrupt, which reads the event ID from shared memory, and wakes a
thread blocked on the corresponding event file. If no threads are
blocked on the file, some state is set in the module such that the next
time a process waits on that file, it returns immediately and clears the
state, mimicking the behaviour of notifications.

## Exercises

In this tutorial you will
create a program that runs in the guest, and sends a string
to a CAmkES component to output. To achieve this, the guest program will write a string
to a shared buffer between itself and a CAmkES component. When its ready
for the string to be printed, it will emit an event, received by the
CAmkES component. The CAmkES component will print the string, then send
an event to the guest process so the guest knows it's safe to send a new
string.

### Add modules to the guest

There is a library in `projects/camkes/vm-linux/camkes-linux-artifacts/camkes-linux-apps/camkes-connector-apps/libs`
containing Linux system call wrappers, and some utility programs in
`projects/camkes/vm-linux/camkes-linux-artifacts/camkes-linux-apps/camkes-connector-apps/pkgs/{dataport,consumes_event,emits_event}`
which initialize and interact with cross VM connections. To build and use these modules in your rootfs the vm-linux
project provides an overlay target you can use.

**Exercise** First add the `dataport`, `consumes_event` and `emits_event` kernel modules to the  rootfs in the guest.

Start by replacing the line:

```cmake
/*-- filter TaskContent("crossvm", TaskContentType.BEFORE, subtask="vm-cmake-start", completion='buildroot login') -*/
AddToFileServer("rootfs.cpio" ${default_rootfs_file})
/*-- endfilter -*/
```

in the target applications `CMakeLists.txt` file with the following:

```cmake
/*-- filter TaskContent("crossvm", TaskContentType.COMPLETED, subtask="vm-cmake-start", completion='buildroot login') -*/
set(CAmkESVMDefaultBuildrootOverlay ON CACHE BOOL "" FORCE)
AddOverlayDirToRootfs(default_buildroot_overlay ${default_rootfs_file} "buildroot" "rootfs_install"
    rootfs_file rootfs_target)
AddToFileServer("rootfs.cpio" ${rootfs_file})
/*-- endfilter -*/
```

### Define interfaces in the VMM

**Exercise** Update the CAmkES file, `crossvm_tutorial.camkes` by replacing the Init0 component definition:

```c
/*-- filter TaskContent("crossvm", TaskContentType.BEFORE, subtask="vm-camkes-init0") -*/
component Init0 {
    VM_INIT_DEF()
}
/*-- endfilter -*/
```

with the following definition:

```c
/*-- filter TaskContent("crossvm", TaskContentType.COMPLETED, subtask="vm-camkes-init0", completion='buildroot login') -*/
component Init0 {
  VM_INIT_DEF()

  // this is the data port for shared memory between the component and guest process
  dataport Buf(4096) data;
  // this event tells the component that there is data ready to print
  emits DoPrint do_print;
  // this event tells the guest process that priting is complete
  consumes DonePrinting done_printing;
  // this mutex protects access to shared state between the VMM and the guest Linux
  has mutex cross_vm_event_mutex;
}
/*-- endfilter -*/
```

These interfaces will eventually be made visible to processes running in
the guest linux. The mutex is used to protect access to shared state
between the VMM and guest.

### Define the component interface

**Exercise** Define the print server component by adding the following to
the `crossvm_tutorial.camkes` file, after the `Init0` definition:
```c
/*-- filter TaskContent("crossvm", TaskContentType.COMPLETED, subtask="vm-camkes-printserver", completion='buildroot login') -*/
component PrintServer {
  control;
  dataport Buf(4096) data;
  consumes DoPrint do_print;
  emits DonePrinting done_printing;
}
/*-- endfilter -*/
```

### Instantiate the print server

**Exercise** Replace the `composition` definition:

```c
/*-- filter TaskContent("crossvm", TaskContentType.BEFORE, subtask="vm-camkes-composition", completion='buildroot login') -*/
    composition {
        VM_COMPOSITION_DEF()
        VM_PER_VM_COMP_DEF(0)
    }
/*-- endfilter -*/
```

with the following:

```c
/*-- filter TaskContent("crossvm", TaskContentType.COMPLETED, subtask="vm-camkes-composition", completion='buildroot login') -*/
    composition {
        VM_COMPOSITION_DEF()
        VM_PER_VM_COMP_DEF(0)

        component PrintServer print_server;
        connection seL4Notification conn_do_print(from vm0.do_print,
                                                 to print_server.do_print);
        connection seL4Notification conn_done_printing(from print_server.done_printing,
                                                      to vm0.done_printing);

        connection seL4SharedDataWithCaps conn_data(from print_server.data,
                                                    to vm0.data);
    }
/*-- endfilter -*/
```

The [seL4SharedDataWithCaps][]
connector is a dataport connector much like `seL4SharedData`.
However, the `to` side of the connection also receives access to
the capabilities to the frames backing the dataport, which is required
for cross VM dataports, as the VMM must be able to establish shared memory
at runtime by inserting new mappings into the guest's address space.

[seL4SharedDataWithCaps]: https://docs.sel4.systems/projects/camkes/seL4SharedDataWithCaps.html

**Exercise** Interfaces connected with [seL4SharedDataWithCaps][] must be
configured with an integer specifying the ID and size of the dataport.
 Do this now by modifying `crossvm_tutorial.camkes` with the following
two lines in the configuration section:

```c
    configuration {
    ...
/*-- filter TaskContent("crossvm", TaskContentType.COMPLETED, subtask="vm-camkes-configuration", completion='buildroot login') -*/
        // Add the following 2 lines:
        vm0.data_id = 1; // ids must be contiguous, starting from 1
        vm0.data_size = 4096;
/*-- endfilter -*/
    }
```

### Implement the print server

**Exercise** Add the file `components/print_server.c` with the following contents:
```c
/*-- filter TaskContent("crossvm", TaskContentType.COMPLETED, subtask="vm-printserver", completion='buildroot login') -*/
#include <camkes.h>
#include <stdio.h>

int run(void) {

    while (1) {
        // wait for the next event
        do_print_wait();

        printf("%s\n", (char*)data);

        // signal that we are done printing
        done_printing_emit();
    }

    return 0;
}
/*-- endfilter -*/
```

This provides a very simple component definition that loops forever, printing a string from
shared memory whenever an event is received then emitting an event.
The example code assumes that the shared buffer will contain a valid, null-terminated c string, which is not
 something you should do in practice.

### Implement the VMM side of the connection
Create another c file that tells the VMM about the cross VM connections.
 This file must define 3 functions which initialize each type of cross vm interface:

- `cross_vm_dataports_init`
- `cross_vm_emits_events_init`
- `cross_vm_consumes_events_init`

**Exercise** Add a file `src/cross_vm.c` with the following contents:

```c
/*-- filter TaskContent("crossvm", TaskContentType.COMPLETED, subtask="vm-crossvm-src", completion='buildroot login') -*/
#include <sel4/sel4.h>
#include <camkes.h>
#include <camkes_mutex.h>
#include <camkes_consumes_event.h>
#include <camkes_emits_event.h>
#include <dataport_caps.h>
#include <cross_vm_consumes_event.h>
#include <cross_vm_emits_event.h>
#include <cross_vm_dataport.h>
#include <vmm/vmm.h>
#include <vspace/vspace.h>

// this is defined in the dataport's glue code
extern dataport_caps_handle_t data_handle;

// Array of dataport handles at positions corresponding to handle ids from spec
static dataport_caps_handle_t *dataports[] = {
    NULL, // entry 0 is NULL so ids correspond with indices
    &data_handle,
};

// Array of consumed event callbacks and ids
static camkes_consumes_event_t consumed_events[] = {
    { .id = 1, .reg_callback = done_printing_reg_callback },
};

// Array of emitted event emit functions
static camkes_emit_fn emitted_events[] = {
    NULL,   // entry 0 is NULL so ids correspond with indices
    do_print_emit,
};

// mutex to protect shared event context
static camkes_mutex_t cross_vm_event_mutex = (camkes_mutex_t) {
    .lock = cross_vm_event_mutex_lock,
    .unlock = cross_vm_event_mutex_unlock,
};

int cross_vm_dataports_init(vmm_t *vmm) {
    return cross_vm_dataports_init_common(vmm, dataports, sizeof(dataports)/sizeof(dataports[0]));
}

int cross_vm_emits_events_init(vmm_t *vmm) {
    return cross_vm_emits_events_init_common(vmm, emitted_events,
            sizeof(emitted_events)/sizeof(emitted_events[0]));
}

int cross_vm_consumes_events_init(vmm_t *vmm, vspace_t *vspace, seL4_Word irq_badge) {
    return cross_vm_consumes_events_init_common(vmm, vspace, &cross_vm_event_mutex,
            consumed_events, sizeof(consumed_events)/sizeof(consumed_events[0]), irq_badge);
}
/*-- endfilter -*/
```

### Update the build system

**Exercise** Make the following changes in `CMakeLists.txt` by firstly replacing the declaration of Init0:

```cmake
/*-- filter TaskContent("crossvm", TaskContentType.BEFORE, subtask="vm-cmake-init0", completion='buildroot login') -*/
DeclareCAmkESVM(Init0)
/*-- endfilter -*/
```

with the following declaration:

```cmake
/*-- filter TaskContent("crossvm", TaskContentType.COMPLETED, subtask="vm-cmake-init0", completion='buildroot login') -*/
# Retrieve Init0 cross vm src files
file(GLOB init0_extra src/*.c)
# Declare VM component: Init0
DeclareCAmkESVM(Init0
    EXTRA_SOURCES ${init0_extra}
    EXTRA_LIBS crossvm
)
/*-- endfilter -*/
```

Also add a declaration for a PrintServer component:

```cmake
/*-- filter TaskContent("crossvm", TaskContentType.COMPLETED, subtask="vm-cmake-printserver", completion='buildroot login') -*/
# Declare the CAmkES PrintServer component
DeclareCAmkESComponent(PrintServer SOURCES components/print_server.c)
/*-- endfilter -*/
```

This extends the definition of the Init component with the `cross_vm connector` source and the crossvm
library, and defines the new CAmkES component `PrintServer`.

### Add interfaces to the Guest

**Exercise** Create the following `camkes_init` shell script that is executed as Linux is initialized:

```bash
/*-- filter TaskContent("crossvm", TaskContentType.COMPLETED, subtask="vm-init-crossvm", completion='buildroot login') -*/
#!/bin/sh
# Initialises linux-side of cross vm connections.

# Dataport sizes must match those in the camkes spec.
# For each argument to dataport_init, the nth pair
# corresponds to the dataport with id n.
dataport_init /dev/camkes_data 4096

# The nth argument to event_init corresponds to the
# event with id n according to the camkes vmm.
consumes_event_init /dev/camkes_done_printing
emits_event_init /dev/camkes_do_print
/*-- endfilter -*/
```

Each of these commands creates device nodes associated with a particular
Linux kernel module supporting cross VM communication. Each command
takes a list of device nodes to create, which must correspond to the IDs
assigned to interfaces in `crossvm_tutorial.camkes` and `cross_vm.c`. The
`dataport_init` command must also be passed the size of each dataport.

These changes will cause device nodes to be created which correspond to
the interfaces you added to the VMM component.

### Create a process

Now make a process that uses the device nodes to communicate with the
print server.

**Exercise** First create a new directory:
```
mkdir -p pkgs/print_client
```

with the following file `pkgs/print_client/print_client.c`:

```c
/*-- filter TaskContent("crossvm", TaskContentType.COMPLETED, subtask="vm-pkg-print_client-src", completion='buildroot login') -*/
#include <string.h>
#include <assert.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>

#include "dataport.h"
#include "consumes_event.h"
#include "emits_event.h"

int main(int argc, char *argv[]) {

    int data_fd = open("/dev/camkes_data", O_RDWR);
    assert(data_fd >= 0);

    int do_print_fd = open("/dev/camkes_do_print", O_RDWR);
    assert(do_print_fd >= 0);

    int done_printing_fd = open("/dev/camkes_done_printing", O_RDWR);
    assert(done_printing_fd >= 0);

    char *data = (char*)dataport_mmap(data_fd);
    assert(data != MAP_FAILED);

    ssize_t dataport_size = dataport_get_size(data_fd);
    assert(dataport_size > 0);

    for (int i = 1; i < argc; i++) {
        strncpy(data, argv[i], dataport_size);
        emits_event_emit(do_print_fd);
        consumes_event_wait(done_printing_fd);
    }

    close(data_fd);
    close(do_print_fd);
    close(done_printing_fd);

    return 0;
}
/*-- endfilter -*/
```

This program prints each of its arguments on a separate line, by sending
each argument to the print server one at a time.

**Exercise** Create `pkgs/print_client/CMakeLists.txt` for our client program:

```cmake
/*-- filter TaskContent("crossvm", TaskContentType.COMPLETED, subtask="vm-pkg-print_client-cmake", completion='buildroot login') -*/
cmake_minimum_required(VERSION 3.8.2)

project(print_client C)

file(READ ${CMAKE_MODULE_PATH_FILE} module_path)
list(APPEND CMAKE_MODULE_PATH ${module_path})
find_package(camkes-vm-linux REQUIRED)
add_subdirectory(${CAMKES_VM_LINUX_DIR}/camkes-linux-artifacts/camkes-linux-apps/camkes-connector-apps/libs/camkes camkes)

add_executable(print_client print_client.c)
target_link_libraries(print_client camkeslinux)
/*-- endfilter -*/
```

**Exercise** Update our the VM apps `CMakeLists.txt`. Below the line:

```cmake
AddToFileServer("bzimage" ${decompressed_kernel} DEPENDS extract_linux_kernel)
```

add the `ExternalProject` declaration to include the print application:

```cmake
/*-- filter TaskContent("crossvm", TaskContentType.COMPLETED, subtask="vm-cmake-printclient-proj", completion='buildroot login') -*/
# Get Custom toolchain for 32 bit Linux
include(cross_compiling)
FindCustomPollyToolchain(LINUX_32BIT_TOOLCHAIN "linux-gcc-32bit-pic")
# Declare our print server app external project
include(ExternalProject)
file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/module_path "${CMAKE_MODULE_PATH}")
ExternalProject_Add(print_client-app
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/pkgs/print_client
    BINARY_DIR ${CMAKE_BINARY_DIR}/print_client-app
    BUILD_ALWAYS ON
    STAMP_DIR ${CMAKE_CURRENT_BINARY_DIR}/print_client-app-stamp
    EXCLUDE_FROM_ALL
    INSTALL_COMMAND ""
    CMAKE_ARGS
        -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
        -DCMAKE_TOOLCHAIN_FILE=${LINUX_32BIT_TOOLCHAIN}
        -DCMAKE_MODULE_PATH_FILE=${CMAKE_CURRENT_BINARY_DIR}/module_path
)
# Add the print client app to our overlay ('default_buildroot_overlay')
AddExternalProjFilesToOverlay(print_client-app ${CMAKE_BINARY_DIR}/print_client-app default_buildroot_overlay "usr/sbin"
    FILES print_client)
/*-- endfilter -*/
```

Directly below this we also want to add our `camkes_init` script into the overlay. We place this into the VMs `init.d` directory so
the script is run on start up:

```cmake
/*-- filter TaskContent("crossvm", TaskContentType.COMPLETED, subtask="vm-cmake-camkes_init", completion='buildroot login') -*/
AddFileToOverlayDir("S90camkes_init" ${CMAKE_CURRENT_LIST_DIR}/camkes_init "etc/init.d" default_buildroot_overlay)
/*-- endfilter -*/
```

That's it. Build and run the system, and you should see the following output:

```
...
Creating dataport node /dev/camkes_data
Allocating 4096 bytes for /dev/camkes_data
Creating consuming event node /dev/camkes_done_printing
Creating emitting event node /dev/camkes_do_print

Welcome to Buildroot
buildroot login: root
Password:
# print_client hello world
[   12.730073] dataport received mmap for minor 1
hello
world
```

/*- filter ExcludeDocs() -*/

```cmake
/*- filter File("CMakeLists.txt") -*/
include(${SEL4_TUTORIALS_DIR}/settings.cmake)
sel4_tutorials_regenerate_tutorial(${CMAKE_CURRENT_SOURCE_DIR})

cmake_minimum_required(VERSION 3.8.2)

project(vm-app C ASM)
find_package(camkes-vm REQUIRED)
include(${CAMKES_VM_SETTINGS_PATH})
camkes_x86_vm_setup_x86_vm_environment()
include(${CAMKES_VM_HELPERS_PATH})
find_package(camkes-vm-linux REQUIRED)
include(${CAMKES_VM_LINUX_HELPERS_PATH})

include(simulation)
GenerateSimulateScript()

# Check kernel config options
if(NOT "${KernelX86Sel4Arch}" STREQUAL "x86_64")
    message(FATAL_ERROR "This application is only supported on x86_64")
endif()
if(NOT "${KernelMaxNumNodes}" STREQUAL 1)
    message(FATAL_ERROR "KernelMaxNumNodes must be set to 1 but are set to:${KernelMaxNumNodes}")
endif()

/*? include_task_type_append([("crossvm", "vm-cmake-init0")]) ?*/
/*? include_task_type_append([("crossvm", "vm-cmake-printserver")]) ?*/
# Get Default Linux VM files
GetArchDefaultLinuxKernelFile("32" default_kernel_file)
GetArchDefaultLinuxRootfsFile("32" default_rootfs_file)

# Decompress Linux Kernel image and add to file server
DecompressLinuxKernel(extract_linux_kernel decompressed_kernel ${default_kernel_file})

AddToFileServer("bzimage" ${decompressed_kernel} DEPENDS extract_linux_kernel)

/*? include_task_type_append([("crossvm", "vm-cmake-printclient-proj")]) ?*/
/*? include_task_type_append([("crossvm", "vm-cmake-camkes_init")]) ?*/
/*? include_task_type_append([("crossvm", "vm-cmake-start")]) ?*/
# Initialise CAmkES Root Server with addition CPP includes
DeclareCAmkESVMRootServer(crossvm_tutorial.camkes)
GenerateCAmkESRootserver()
/*- endfilter -*/
```

```c
/*- filter File("crossvm_tutorial.camkes") --*/
import <VM/vm.camkes>;

#include <configurations/vm.h>

#define VM_GUEST_CMDLINE "earlyprintk=ttyS0,115200 console=ttyS0,115200 i8042.nokbd=y i8042.nomux=y \
i8042.noaux=y io_delay=udelay noisapnp pci=nomsi debug root=/dev/mem"

/*? include_task_type_append([("crossvm", "vm-camkes-init0")]) ?*/
/*? include_task_type_append([("crossvm", "vm-camkes-printserver")]) ?*/
assembly {

    /*? include_task_type_append([("crossvm", "vm-camkes-composition")]) ?*/
    configuration {
        VM_CONFIGURATION_DEF()
        VM_PER_VM_CONFIG_DEF(0)

        vm0.simple_untyped23_pool = 20;
        vm0.heap_size = 0x2000000;
        vm0.guest_ram_mb = 128;
        vm0.kernel_cmdline = VM_GUEST_CMDLINE;
        vm0.kernel_image = "bzimage";
        vm0.kernel_relocs = "bzimage";
        vm0.initrd_image = "rootfs.cpio";
        vm0.iospace_domain = 0x0f;
    /*? include_task_type_append([("crossvm", "vm-camkes-configuration")]) ?*/
    }
}
/*- endfilter -*/
```

```c
/*- filter File("components/print_server.c") --*/
/*? include_task_type_append([("crossvm", "vm-printserver")]) ?*/
/*- endfilter -*/
```

```c
/*- filter File("src/cross_vm.c") --*/
/*? include_task_type_append([("crossvm", "vm-crossvm-src")]) ?*/
/*- endfilter -*/
```

```bash
/*- filter File("camkes_init", mode="executable") --*/
/*? include_task_type_append([("crossvm", "vm-init-crossvm")]) ?*/
/*- endfilter -*/
```

```c
/*- filter File("pkgs/print_client/print_client.c") --*/
/*? include_task_type_append([("crossvm", "vm-pkg-print_client-src")]) ?*/
/*- endfilter -*/
```

```cmake
/*- filter File("pkgs/print_client/CMakeLists.txt") --*/
/*? include_task_type_append([("crossvm", "vm-pkg-print_client-cmake")]) ?*/
/*- endfilter -*/
```

/*- endfilter -*/
