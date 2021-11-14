<!--
  Copyright 2017, Data61, CSIRO (ABN 41 687 119 230)

  SPDX-License-Identifier: BSD-2-Clause
-->

/*? declare_task_ordering(['vm-cmake-start','vm-pkg-hello-c','vm-pkg-hello-cmake','vm-cmake-hello','vm-module-poke-c','vm-module-poke-make','vm-module-poke-cmake','vm-cmake-poke','vm-init-poke']) ?*/

# CAmkES VM: Adding a Linux Guest

This tutorial provides an introduction to creating VM guests and applications on seL4 using CAmkES.

## Prerequisites

1. [Set up your machine](https://docs.sel4.systems/HostDependencies#camkes-build-dependencies).
1. [Camkes VM](https://docs.sel4.systems/Tutorials/camkes-vm-linux)

## Outcomes

By the end of this tutorial, you should be familiar with:

* Creating, configuring and building guest Linux VM components in CAmkES.
* Building and installing your own Linux VM user-level programs and kernel modules.

## Background

This tutorial is set up with a basic CAmkES VM configuration for you to build upon.
The starting application should boot a single, very basic Linux guest.

To build the tutorial, run:
/*? macros.ninja_block() ?*/

You can boot the tutorial on an x86 hardware platform with a multiboot boot loader, 
or use the [QEMU](https://www.qemu.org) simulator. **Note if you are using QEMU
it is important to ensure that your host machine has VT-x support and [KVM](https://www.linux-kvm.org/page/Main_Page)
installed. You also need to ensure you have enabled nested virtulisation with KVM guests as described
[here](https://www.linux-kvm.org/page/Nested_Guests).**

To simulate the image you can run the provided simulation script with some additional parameters:

```sh
# In the build directory
# You will need to set up a tap device first:
# ip tuntap add tap0 mode tap
# ip addr add 10.0.120.100/24 dev tap0
# ip link set dev tap0 up
sudo ./simulate --machine q35,accel=kvm,kernel-irqchip=split --mem-size 2G --extra-cpu-opts "+vmx" --extra-qemu-args="-enable-kvm -device intel-iommu,intremap=off -net nic,model=e1000 -net tap,script=no,ifname=tap0"
```

When first simulating the image you should see the following login prompt:
```
Welcome to Buildroot
buildroot login:
```

You can login with the username `root` and the password `root`.

The Linux guest was built using [buildroot](https://buildroot.org/), which 
creates a compatible kernel and minimal root filesystem containing busybox and a in-memory file system (a ramdisk).

## VM Components

Each VM component has its own assembly implementation, where the guest environment is configured.
The provided VM configuration is defined in `vm_tutorial.camkes`:

```
/*-- filter File("vm_tutorial.camkes") -*/
import <VM/vm.camkes>;

#include <configurations/vm.h>

#define VM_GUEST_CMDLINE "earlyprintk=ttyS0,115200 console=ttyS0,115200 i8042.nokbd=y i8042.nomux=y \
i8042.noaux=y io_delay=udelay noisapnp pci=nomsi debug root=/dev/mem"

component Init0 {
    VM_INIT_DEF()
}

assembly {
    composition {
        VM_COMPOSITION_DEF()
        VM_PER_VM_COMP_DEF(0)
    }

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
    }
}
/*-- endfilter -*/
```

Most of the work here is done by five C preprocessor macros:
`VM_INIT_DEF`, `VM_COMPOSITION_DEF`, `VM_PER_VM_COMP_DEF`,
`VM_CONFIGURATION_DEF`, `VM_PER_VM_CONFIG_DEF`.

These are all defined in `projects/camkes/vm/components/VM/configurations/vm.h`,
and are concerned with specifying and configuring components that all
VM(M)s need.

The `Init0` component corresponds to a single guest. Because of some rules
in the cpp macros, the *Ith* guest in your system must be defined as a
component named `InitI`. `InitI` components will be instantiated in the
composition section by the `VM_PER_VM_COMP_DEF` macro with instance
names `vmI`. The `vm0` component instance being configured above is an
instance of `Init0`. The C source code for`InitI` components is in
`projects/camkes/vm/components/Init/src`. This source will be used for components
named `InitI` for *I* in `0..VM_NUM_VM - 1`.

The values of `vm0.kernel_cmdline`, `vm0.kernel_image` and `vm0.initrd_image` are all
strings specifying:
 - boot arguments to the guest Linux,
 - the name of the guest Linux kernel image file,
 - and the name of the guest Linux initrd file (the root filesystem to use during system initialization).
 
The kernel command-line is defined in the `VM_GUEST_CMDLINE` macro. The kernel image
and rootfs names are defined in the applications `CMakeLists.txt` file.
These are the names of files in a CPIO archive that gets created by the build system, and
linked into the VMM. In the simple configuration for thie tutorial, the VMM uses
the `bzimage` and `rootfs.cpio` names to find the appropriate files
in this archive.

To see how the `Init` component and CPIO archive are definied within the build system,
look at the app's `CMakeList.txt`:

```cmake
include(${SEL4_TUTORIALS_DIR}/settings.cmake)
sel4_tutorials_regenerate_tutorial(${CMAKE_CURRENT_SOURCE_DIR})

cmake_minimum_required(VERSION 3.8.2)

project(vm-app C ASM)
include(ExternalProject)
find_package(camkes-vm REQUIRED)
include(${CAMKES_VM_SETTINGS_PATH})
camkes_x86_vm_setup_x86_vm_environment()
include(${CAMKES_VM_HELPERS_PATH})
find_package(camkes-vm-linux REQUIRED)
include(${CAMKES_VM_LINUX_HELPERS_PATH})

/*- filter TaskContent("vm-cmake-start", TaskContentType.ALL, subtask='includes', completion='buildroot login') -*/
# Include CAmkES VM helper functions
/*- endfilter -*/

/*- filter TaskContent("vm-cmake-start", TaskContentType.ALL, subtask='pre_rootfs', completion='buildroot login') -*/

# Declare VM component: Init0
DeclareCAmkESVM(Init0)

# Get Default Linux VM files
GetArchDefaultLinuxKernelFile("32" default_kernel_file)
GetArchDefaultLinuxRootfsFile("32" default_rootfs_file)

# Decompress Linux Kernel image and add to file server
DecompressLinuxKernel(extract_linux_kernel decompressed_kernel ${default_kernel_file})

AddToFileServer("bzimage" ${decompressed_kernel} DEPENDS extract_linux_kernel)
/*- endfilter -*/

/*- filter TaskContent("vm-cmake-start", TaskContentType.ALL, subtask='default', completion='buildroot login') -*/
# Add rootfs images into file server
AddToFileServer("rootfs.cpio" ${default_rootfs_file})
/*- endfilter -*/

/*- filter TaskContent("vm-cmake-start", TaskContentType.ALL, subtask='post_rootfs', completion='buildroot login') -*/
# Initialise CAmkES Root Server with addition CPP includes
DeclareCAmkESVMRootServer(vm_tutorial.camkes)
GenerateCAmkESRootserver()
/*- endfilter -*/
```

The file `projects/camkes/vm/camkes_vm_helpers.cmake` provides helper functions for the VM projects, 
including  `DeclareCAmkESVM(Init0)`, which is used to define the `Init0` VM component.
Each Init component requires a corresponding `DeclareCAmkESVM` function.

`GetArchDefaultLinuxKernelFile` (defined in `projects/camkes/vm-linux/vm-linux-helpers.cmake`)
is a helper function that retrieves the location of an architectural specific VM image provided
in the `projects/vm-linux` folder, which contains some tools for building new linux kernel
and root filesystem images, as well as the images that these tools
produce. A fresh checkout of this project will contain some pre-built
images (`bzimage` and `rootfs.cpio`), to speed up build times.
 
`DecompressLinuxKernel` is used to extract the vmlinux image, which `AddToFileServer` then places 
in the fileserver along with the rootfs.
 
## Adding to the guest

In the simple buildroot guest image, the
initrd (rootfs.cpio) is also the filesystem you get access to after
logging in. To make new programs available to the guest you need to add them to the
rootfs.cpio archive. Similarly, to make new kernel modules available to
the guest they must be added to the rootfs.cpio archive also. 

In this tutorial you will  install new programs into the guest VM.

### vm-linux-helpers.cmake

The `projects/camkes/vm-linux` directory contains CMake helpers to
overlay rootfs.cpio archives with a desired set of programs, modules
and scripts. 

#### `AddFileToOverlayDir(filename file_location root_location overlay_name)`
This helper allows you to overlay specific files onto a rootfs image. The caller specifies
the file they wish to install in the rootfs image (`file_location`), the name they want the file
to be called in the rootfs (`filename`) and the location they want the file to installed in the
rootfs (`root_location`), e.g "usr/bin". Lastly the caller passes in a unique target name for the overlay
(`overlay_name`). You can repeatedly call this helper with different files for a given target to build
up a set of files to be installed on a rootfs image.

#### `AddOverlayDirToRootfs(rootfs_overlay rootfs_image rootfs_distro rootfs_overlay_mode output_rootfs_location target_name)`
This helper allows you to install a defined overlay target onto a given rootfs image. The caller specifies
the rootfs overlay target name (`rootfs_overlay`), the rootfs image they wish to install their files onto
(`rootfs_image`), the distribution of their rootfs image (`rootfs_distro`, only 'buildroot' and 'debian' is
supported) and the output location of their overlayed rootfs image (`output_rootfs_location`). Lastly the caller
specifies how the files will be installed into their rootfs image through `rootfs_overlay_mode`. These modes include:
* `rootfs_install`: The files are installed onto the rootfs image. This is useful if the rootfs image is the filesystem
your guest VM is using when it boots. However this won't be useful if your VM will be booting from disk since the installed files
won't be present after the VM boots.
* `overlay`: The files are mounted as an overlayed filesystem (overlayfs). This is useful if you are booting from disk and don't wish to
install the artifacts permanently onto the VM. The downside to this is that writes to the overlayed root do not persist between boots. This
mode is benefitial for debugging purposes and live VM images.
* `fs_install`: The files are permanently installed on the VM's file system, after the root has been mounted.
#### `AddExternalProjFilesToOverlay(external_target external_install_dir overlay_target overlay_root_location)`
This helper allows you to add files generated from an external CMake project to an overlay target. This is mainly a wrapper around
`AddOverlayDirToRootfs` which in addition creates a target for the generated file in the external project. The caller passes the external
project target (`external_target`), the external projects install directory (`external_install_dir`), the overlay target you want to add the
file to (`overlay_target`) and the location you wish to install the file within the rootfs image (`overlay_root_location`).

### linux-source-helpers.cmake

#### `DownloadLinux(linux_major linux_minor linux_md5 linux_out_dir linux_out_target)`
This is a helper function for downloading the linux source. This is needed if we wish to build our own kernel modules.

#### `ConfigureLinux(linux_dir linux_config_location linux_symvers_location configure_linux_target)`
This helper function is used for configuring downloaded linux source with a given Kbuild defconfig (`linux_config_location`)
and symvers file (`linux_symvers_location`).

## Exercises 

### Adding a program

This exercise guides you through adding a new program to the Linux guest user-level environment.

First, make a new directory:
```bash
mkdir -p pkg/hello
```
Then a simple C program in `pkg/hello/hello.c`:
```c
/*-- filter TaskContent("vm-pkg-hello-c", TaskContentType.COMPLETED, completion='buildroot login') -*/
#include <stdio.h>

int main(int argc, char *argv[]) {
    printf("Hello, World!\n");
    return 0;
}
/*-- endfilter -*/
```
Then create a build file for the program at `pkg/hello/CMakeLists.txt`:

```cmake
/*-- filter TaskContent("vm-pkg-hello-cmake", TaskContentType.COMPLETED, completion='buildroot login') -*/
cmake_minimum_required(VERSION 3.8.2)

project(hello C)

add_executable(hello hello.c)

target_link_libraries(hello -static)
/*-- endfilter -*/
```
Now integrate the new program with the build system. 
Update the VM apps `CMakeLists.txt` to declare the hello application as an
external project and add it to our overlay.
 Do this by replacing the line `AddToFileServer("rootfs.cpio" ${default_rootfs_file})` with the following:

```cmake
/*-- filter TaskContent("vm-cmake-hello", TaskContentType.COMPLETED, subtask='toolchain', completion='buildroot login') -*/
# Get Custom toolchain for 32 bit Linux
include(cross_compiling)
FindCustomPollyToolchain(LINUX_32BIT_TOOLCHAIN "linux-gcc-32bit-pic")
/*-- endfilter -*/
/*-- filter TaskContent("vm-cmake-hello", TaskContentType.COMPLETED, subtask='hello', completion='buildroot login') -*/
# Declare our hello app external project
ExternalProject_Add(hello-app
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/pkg/hello
    BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/hello-app
    BUILD_ALWAYS ON
    STAMP_DIR ${CMAKE_CURRENT_BINARY_DIR}/hello-app-stamp
    EXCLUDE_FROM_ALL
    INSTALL_COMMAND ""
    CMAKE_ARGS
        -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
        -DCMAKE_TOOLCHAIN_FILE=${LINUX_32BIT_TOOLCHAIN}
)
# Add the hello world app to our overlay ('vm-overlay')
AddExternalProjFilesToOverlay(hello-app ${CMAKE_CURRENT_BINARY_DIR}/hello-app vm-overlay "usr/sbin"
    FILES hello)
# Add the overlay directory to our default rootfs image
AddOverlayDirToRootfs(vm-overlay ${default_rootfs_file} "buildroot" "rootfs_install"
    rootfs_file rootfs_target)
AddToFileServer("rootfs.cpio" ${rootfs_file} DEPENDS rootfs_target)
/*-- endfilter -*/
```
Now rebuild the project...
/*? macros.ninja_block() ?*/
..and run it (use `root` as username and password).
You should be able to use the new program. 

```
Welcome to Buildroot
buildroot login: root
Password:
# hello
Hello, World!
```

## Adding a kernel module

The next exercise guides you through the addition of a new kernel module that provides
guest to VMM communication. This is a very simply module: you'll create a special file
associated with the new module, which when written to causes the VMM to print message.

First, make a new directory:
```
mkdir -p modules/poke
```
Then create the following file for the module in `modules/poke/poke.c`.
```c
/*-- filter TaskContent("vm-module-poke-c", TaskContentType.COMPLETED, completion='buildroot login') -*/
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/fs.h>

#include <asm/uaccess.h>
#include <asm/kvm_para.h>
#include <asm/io.h>

#define DEVICE_NAME "poke"

static int major_number;

static ssize_t poke_write(struct file *f, const char __user*b, size_t s, loff_t *o) {
    printk("hi\n"); // TODO replace with hypercall
    return s;
}

struct file_operations fops = {
    .write = poke_write,
};

static int __init poke_init(void) {
    major_number = register_chrdev(0, DEVICE_NAME, &fops);
    printk(KERN_INFO "%s initialized with major number %dn", DEVICE_NAME, major_number);
    return 0;
}

static void __exit poke_exit(void) {
    unregister_chrdev(major_number, DEVICE_NAME);
    printk(KERN_INFO"%s exitn", DEVICE_NAME);
}

module_init(poke_init);
module_exit(poke_exit);
/*-- endfilter -*/
```
Now add a Makefile for building the module in `modules/poke/Makefile`:

```make
/*-- filter TaskContent("vm-module-poke-make", TaskContentType.COMPLETED, completion='buildroot login') -*/
obj-m += poke.o

all:
/*?tab-?*/make -C $(KHEAD) M=$(PWD) modules

clean:
/*?tab-?*/make -C $(KHEAD) M=$(PWD) clean
/*-- endfilter -*/
```

Create a `modules/CMakeLists.txt` to define the new Linux module with the following content:
```cmake
/*-- filter TaskContent("vm-module-poke-cmake", TaskContentType.COMPLETED, completion='buildroot login') -*/
cmake_minimum_required(VERSION 3.8.2)

if(NOT MODULE_HELPERS_FILE)
    message(FATAL_ERROR "MODULE_HELPERS_FILE is not defined")
endif()

include("${MODULE_HELPERS_FILE}")

DefineLinuxModule(${CMAKE_CURRENT_LIST_DIR}/poke poke-module poke-target KERNEL_DIR ${LINUX_KERNEL_DIR})
/*-- endfilter -*/
```
Update the VM `CMakeLists.txt` file to declare the new poke module as an
external project and add it to the overlay. 

At the top of the file include our linux helpers, add the following:

```cmake
/*-- filter TaskContent("vm-cmake-poke", TaskContentType.COMPLETED, subtask='includes', completion='buildroot login') -*/
include(${CAMKES_VM_LINUX_SOURCE_HELPERS_PATH})
/*-- endfilter -*/
```
Below the includes (before `AddOverlayDirToRootfs` that was added in the first exercise **Adding a program**) add:
```cmake
/*-- filter TaskContent("vm-cmake-poke", TaskContentType.COMPLETED, subtask='module', completion='buildroot login') -*/
# Setup Linux Sources
GetDefaultLinuxMajor(linux_major)
GetDefaultLinuxMinor(linux_minor)
GetDefaultLinuxMd5(linux_md5)
# Download and Configure our Linux sources
DownloadLinux(${linux_major} ${linux_minor} ${linux_md5} vm_linux_extract_dir download_vm_linux)
set(linux_config "${CAMKES_VM_LINUX_DIR}/linux_configs/${linux_major}.${linux_minor}/32/config")
set(linux_symvers "${CAMKES_VM_LINUX_DIR}/linux_configs/${linux_major}.${linux_minor}/32/Module.symvers")
ConfigureLinux(${vm_linux_extract_dir} ${linux_config} ${linux_symvers} configure_vm_linux
    DEPENDS download_vm_linux
)
# Add the external poke module project
ExternalProject_Add(poke-module
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/modules
    BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/poke-module
    BUILD_ALWAYS ON
    STAMP_DIR ${CMAKE_CURRENT_BINARY_DIR}/poke-module-stamp
    EXCLUDE_FROM_ALL
    INSTALL_COMMAND ""
    DEPENDS download_vm_linux configure_vm_linux
    CMAKE_ARGS
        -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
        -DCMAKE_TOOLCHAIN_FILE=${LINUX_32BIT_TOOLCHAIN}
        -DLINUX_KERNEL_DIR=${vm_linux_extract_dir}
        -DMODULE_HELPERS_FILE=${CAMKES_VM_LINUX_DIR}/linux-module-helpers.cmake
)
# Add our module binary to the overlay
AddExternalProjFilesToOverlay(poke-module ${CMAKE_CURRENT_BINARY_DIR}/poke-module vm-overlay "lib/modules/4.8.16/kernel/drivers/vmm"
    FILES poke.ko)
/*-- endfilter -*/
```

Write a custom init script that loads the new module during initialization. 
Create a file called `init` in our tutorial directory with the following:

```bash
/*- filter TaskContent("vm-init-poke", TaskContentType.COMPLETED,completion='buildroot login') --*/
#!/bin/sh
# devtmpfs does not get automounted for initramfs
/bin/mount -t devtmpfs devtmpfs /dev
exec 0</dev/console
exec 1>/dev/console
exec 2>/dev/console

insmod /lib/modules/4.8.16/kernel/drivers/vmm/poke.ko
exec /sbin/init $*
/*-- endfilter -*/
```
Now update our the VM apps `CMakeLists.txt` file to add the new init script to the
overlay. After our call to `AddExternalProjFilesToOverlay` and before `AddOverlayDirToRootfs` for the poke module add the following:

```cmake
/*-- filter TaskContent("vm-cmake-poke", TaskContentType.COMPLETED, subtask='init_overlay', completion='buildroot login') -*/
AddFileToOverlayDir("init" ${CMAKE_CURRENT_LIST_DIR}/init "." vm-overlay)
/*-- endfilter -*/
```
and give the script executable permissions:
```bash
chmod +x init
```
Rebuild the project:
/*? macros.ninja_block() ?*/
Run the following commands to see the module being used:

```
Welcome to Buildroot
buildroot login: root
Password:
# grep poke /proc/devices        # figure out the major number of our driver
246 poke
# mknod /dev/poke c 246 0        # create the special file
# echo > /dev/poke               # write to the file
[ 57.389643] hi
-sh: write error: Bad address    # the shell complains, but our module is being invoked!
```

### Create a hypercall

In `modules/poke/poke.c`, replace `printk("hi\n");` with `kvm_hypercall1(4, 0);`.
The choice of 4 is because 0..3 are already used by existing hypercalls.

Then register a handler for this hypercall in `projects/camkes/vm/components/Init/src/main.c`:.
Add a new function at the top of the file:

```c
static int poke_handler(vm_vcpu_t *vm_vcpu) {
    printf("POKE!!!\n");
    return 0;
}
```

In the function `main_continued` register \`poke_handler\`:

```c
vm_reg_new_vmcall_handler(&vm, poke_handler, 4); // <--- added

/* Now go run the event loop */
vmm_run(&vm);
```

Rebuild the project and try out the hypercall + module:
/*? macros.ninja_block() ?*/
```
Welcome to Buildroot
buildroot login: root
Password:
# mknod /dev/poke c 246 0
# echo > /dev/poke
POKE!!!
```
/*-- filter ExcludeDocs() -*/
```cmake
/*- filter File("CMakeLists.txt") -*/
include(${SEL4_TUTORIALS_DIR}/settings.cmake)
sel4_tutorials_regenerate_tutorial(${CMAKE_CURRENT_SOURCE_DIR})

cmake_minimum_required(VERSION 3.8.2)

project(vm-app C ASM)
include(ExternalProject)
find_package(camkes-vm REQUIRED)
include(${CAMKES_VM_SETTINGS_PATH})
camkes_x86_vm_setup_x86_vm_environment()
include(${CAMKES_VM_HELPERS_PATH})
find_package(camkes-vm-linux REQUIRED)
include(${CAMKES_VM_LINUX_HELPERS_PATH})

include(simulation)
GenerateSimulateScript()

/*? include_task_type_append([("vm-cmake-start",'includes')]) ?*/
/*? include_task_type_append([("vm-cmake-poke",'includes')]) ?*/
/*? include_task_type_append([("vm-cmake-start", 'pre_rootfs')]) ?*/
/*? include_task_type_append([("vm-cmake-hello", 'toolchain')]) ?*/
/*? include_task_type_append([("vm-cmake-poke", 'module')]) ?*/
/*? include_task_type_replace([("vm-cmake-start",'default'), ("vm-cmake-hello",'hello')]) ?*/
/*? include_task_type_append([("vm-cmake-poke", 'init_overlay')]) ?*/
/*? include_task_type_append([("vm-cmake-start", 'post_rootfs')]) ?*/
/*- endfilter -*/
```
```c
/*- filter File("pkg/hello/hello.c") -*/
/*? include_task_type_append(["vm-pkg-hello-c"]) ?*/
/*- endfilter -*/
```
```cmake
/*- filter File("pkg/hello/CMakeLists.txt") -*/
/*? include_task_type_append(["vm-pkg-hello-cmake"]) ?*/
/*- endfilter -*/
```
```c
/*- filter File("modules/poke/poke.c") -*/
/*? include_task_type_append(["vm-module-poke-c"]) ?*/
/*- endfilter -*/
```
```make
/*- filter File("modules/poke/Makefile") -*/
/*? include_task_type_append(["vm-module-poke-make"]) ?*/
/*- endfilter -*/
```
```cmake
/*- filter File("modules/CMakeLists.txt") -*/
/*? include_task_type_append(["vm-module-poke-cmake"]) ?*/
/*- endfilter -*/
```
```bash
/*- filter File("init", mode="executable") --*/
/*? include_task_type_append(["vm-init-poke"]) ?*/
/*- endfilter -*/
```

/*-- endfilter -*/
