/*? declare_task_ordering(['vm-cmake-start','vm-pkg-hello-c','vm-pkg-hello-cmake','vm-cmake-hello','vm-module-poke-c','vm-module-poke-make','vm-module-poke-cmake','vm-cmake-poke','vm-init-poke']) ?*/

# CAmkES VM: Adding a Linux Guest

This tutorial provides an introduction to creating VM applications on seL4 using CAmkES.

## Prerequisites

1. [Set up your machine](https://docs.sel4.systems/HostDependencies#camkes-build-dependencies).
2. Familiar with the sytanx, basic structures and file layout of a CAmkES application
3. A basic understanding of building seL4 projects (e.g. using `CMakeLists.txt`)

## Outcomes

By the end of this tutorial, you should be familiar with:

* Creating, configuring and building guest Linux VM components in CAmkES
* Building and installing your own Linux VM user-level programs and kernel modules

## Starting Point
This tutorial is set up with a basic CAmkES VM configuration that we can add to. Our
starting application should boot a single, very basic Linux guest.

To build our starting point we can simply run:
/*? macros.ninja_block() ?*/

We can then either boot our CAmkES VM project on an x86 hardware platform with the multiboot boot loader
of your choice or in an instance of the [QEMU](https://www.qemu.org) simulator. **Note if using QEMU
it is important to ensure that your host machine has VT-x support and [KVM](https://www.linux-kvm.org/page/Main_Page)
installed. It is also important to also ensure you have enabled nested virtulisation with KVM guests as described
[here](https://www.linux-kvm.org/page/Nested_Guests).**

To simulate our image we can run the provided simulation script with some additional parameters:

```sh
# In the build directory
sudo ./simulate --cpu Haswell --machine q35,accel=kvm,kernel-irqchip=split --mem-size 2G --extra-cpu-opts +vmx --extra-qemu-args="-enable-kvm -device intel-iommu,intremap=off"
```

When simulating our image you should see the following login prompt:
```
Welcome to Buildroot
buildroot login:
```

The Linux running here was built using [buildroot](https://buildroot.org/). This tool
creates a compatible kernel and root filesystem with busybox and not
much else, and runs on a ramdisk (the actual hard drive isn't mounted).
Login with the username `root` and the password `root`.

## Looking at the sources

Our starting point is a very simple app, with a single vm, and nothing else. Each
different vm app will have its own assembly implementation, where the guest environment is configured.
Our apps configuration is defined in `vm_tutorial.camkes`:

```
/*- filter File("vm_tutorial.camkes") -*/
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
/*- endfilter -*/
```

Most of the work here is done by five C preprocessor macros:
`VM_INIT_DEF`, `VM_COMPOSITION_DEF`, `VM_PER_VM_COMP_DEF`,
`VM_CONFIGURATION_DEF`, `VM_PER_VM_CONFIG_DEF`

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
strings specifying the guest linux boot arguments, the name of the guest linux kernel image file,
and the name of the guest linux initrd file (root filesystem to use during system initialization).
The kernel command-line is defined in the `VM_GUEST_CMDLINE` macro. The kernel image
and rootfs names will be defined in the applications CMakeLists file.
These are the names of files in a CPIO archive that gets created by the build system, and
linked into the VMM. In our simple app configuration the VMM uses
the `bzimage` and `rootfs.cpio` names to find the appropriate files
in this archive when preparing to boot the guest.

To see how we define our `Init` components and the CPIO archive within the build system we can
look at the app's `CMakeList.txt`:

```cmake
cmake_minimum_required(VERSION 3.8.2)

project(vm-app)

ImportCamkesVM()

/*- filter TaskContent("vm-cmake-start", TaskContentType.ALL, subtask='includes', completion='buildroot login') -*/
# Include CAmkES VM helper functions
include("../projects/camkes/vm/camkes_vm_helpers.cmake")
include("../projects/camkes/vm-linux/vm-linux-helpers.cmake")
/*- endfilter -*/

/*- filter TaskContent("vm-cmake-start", TaskContentType.ALL, subtask='pre_rootfs', completion='buildroot login') -*/
# Define kernel config options
set(KernelX86Sel4Arch x86_64 CACHE STRING "" FORCE)
set(KernelMaxNumNodes 1 CACHE STRING "" FORCE)

# Declare VM component: Init0
DeclareCAmkESVM(Init0)

# Get Default Linux VM files
GetDefaultLinuxKernelFile(default_kernel_file)
GetDefaultLinuxRootfsFile(default_rootfs_file)

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
To access a series of helper functions for defining our CAmkES VM project we need to
include the `projects/camkes/vm/camkes_vm_helpers.cmake` file.
This firstly enables us to call `DeclareCAmkESVM(Init0)` to define our `Init0` VM component.
With each Init component we define in the CAmkES configuration we need to correspondingly define
the component with the `DeclareCAmkESVM` function.

To find a kernel and rootfs image we use the `GetDefaultLinuxKernelFile` helper (defined in
`projects/camkes/vm-linux/vm-linux-helpers.cmake`) to retrieve the location of the vm images provided
in the `projects/vm-linux` folder. This project contains some tools for building new linux kernel
and root filesystem images, as well as the images that these tools
produce. A fresh checkout of this project will contain some pre-built
images (`bzimage` and `rootfs.cpio`), to speed up build times. We call the
`DecompressLinuxKernel` helper to extract the vmlinux image. Lastly we add the
decompressed kernel image and rootfs to the fileserver through the `AddToFileServer` helper. These are
placed in the file server under the names we wish to access them
by in the archive. In our case this is `bzimage` and `rootfs.cpio`.

## Adding to the guest

In the simple buildroot guest image, the
initrd (rootfs.cpio) is also the filesystem you get access to after
logging in. To make new programs available to the guest we need to add them to the
rootfs.cpio archive. Similarly, to make new kernel modules available to
the guest they must be added to the rootfs.cpio archive also. In this task we will look
at how we can install new programs into our guest VMs.

### Background

The `projects/camkes/vm-linux` directory contains CMake helpers to
overlay rootfs.cpio archives with a desired set of programs, modules
and scripts.

Here's a summary of some of the CMake helpers that are available to help you add your own files
to the rootfs image:

#### vm-linux-helpers.cmake

##### `AddFileToOverlayDir(filename file_location root_location overlay_name)`
This helper allows you to overlay specific files onto a rootfs image. The caller specifies
the file they wish to install in the rootfs image (`file_location`), the name they want the file
to be called in the rootfs (`filename`) and the location they want the file to installed in the
rootfs (`root_location`), e.g "usr/bin". Lastly the caller passes in a unique target name for the overlay
(`overlay_name`). You can repeatedly call this helper with different files for a given target to build
up a set of files to be installed on a rootfs image.

##### `AddOverlayDirToRootfs(rootfs_overlay rootfs_image rootfs_distro rootfs_overlay_mode output_rootfs_location target_name)`
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
##### `AddExternalProjFilesToOverlay(external_target external_install_dir overlay_target overlay_root_location)`
This helper allows you to add files generated from an external CMake project to an overlay target. This is mainly a wrapper around
`AddOverlayDirToRootfs` which in addition creates a target for the generated file in the external project. The caller passes the external
project target (`external_target`), the external projects install directory (`external_install_dir`), the overlay target you want to add the
file to (`overlay_target`) and the location you wish to install the file within the rootfs image (`overlay_root_location`).

#### linux-source-helpers.cmake

##### `DownloadLinux(linux_major linux_minor linux_md5 linux_out_dir linux_out_target)`
This is a helper function for downloading the linux source. This is needed if we wish to build our own kernel modules.

##### `ConfigureLinux(linux_dir linux_config_location linux_symvers_location configure_linux_target)`
This helper function is used for configuring downloaded linux source with a given Kbuild defconfig (`linux_config_location`)
and symvers file (`linux_symvers_location`).

## Adding a program

 Let's add a simple program!

1.  Make a new directory:

```bash
mkdir -p pkg/hello
```

2.  Add a simple C program in `pkg/hello/hello.c`:

```c
/*- filter TaskContent("vm-pkg-hello-c", TaskContentType.COMPLETED, completion='buildroot login') -*/
#include <stdio.h>

int main(int argc, char *argv[]) {
    printf("Hello, World!\n");
    return 0;
}
/*- endfilter -*/
```

3.  We want to target our application to run in a 32-bit x86 Linux VM. To achieve this we need to build our application as an external
project. Firstly we need to add a CMake file at `pkg/hello/CMakeLists.txt`:

```cmake
/*- filter TaskContent("vm-pkg-hello-cmake", TaskContentType.COMPLETED, completion='buildroot login') -*/
cmake_minimum_required(VERSION 3.8.2)

project(hello C)

add_executable(hello hello.c)

target_link_libraries(hello -static)
/*- endfilter -*/
```

4.  Update our vm apps CMakeList file (CMakeLists.txt) to declare our hello application as an
external project and add it to our overlay. Replace the line `AddToFileServer("rootfs.cpio" ${default_rootfs_file})` with the following:

```cmake
/*- filter TaskContent("vm-cmake-hello", TaskContentType.COMPLETED, subtask='toolchain', completion='buildroot login') -*/
# Get Custom toolchain for 32 bit Linux
FindCustomPollyToolchain(LINUX_32BIT_TOOLCHAIN "linux-gcc-32bit-pic")
/*- endfilter -*/
/*- filter TaskContent("vm-cmake-hello", TaskContentType.COMPLETED, subtask='hello', completion='buildroot login') -*/
# Declare our hello app external project
ExternalProject_Add(hello-app
    URL file:///${CMAKE_CURRENT_SOURCE_DIR}/pkg/hello
    BINARY_DIR ${CMAKE_BINARY_DIR}/hello-app
    BUILD_ALWAYS ON
    STAMP_DIR ${CMAKE_CURRENT_BINARY_DIR}/hello-app-stamp
    EXCLUDE_FROM_ALL
    INSTALL_COMMAND ""
    CMAKE_ARGS
        -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
        -DCMAKE_TOOLCHAIN_FILE=${LINUX_32BIT_TOOLCHAIN}
)
# Add the hello world app to our overlay ('vm-overlay')
AddExternalProjFilesToOverlay(hello-app ${CMAKE_BINARY_DIR}/hello-app vm-overlay "usr/sbin"
    FILES hello)
# Add the overlay directory to our default rootfs image
AddOverlayDirToRootfs(vm-overlay ${default_rootfs_file} "buildroot" "rootfs_install"
    rootfs_file rootfs_target)
AddToFileServer("rootfs.cpio" ${rootfs_file} DEPENDS rootfs_target)
/*- endfilter -*/
```

5.  Rebuild the app:

/*? macros.ninja_block() ?*/

6.  Run the app (use `root` as username and password):

```
Welcome to Buildroot
buildroot login: root
Password:
# hello
Hello, World!
```

## Adding a kernel module

We're going to add a new kernel module that lets us poke the vmm.

1.  Make a new directory:

```
mkdir -p modules/poke
```

2.  Implement the module in `modules/poke/poke.c`.

Initially we'll just get the module building and running, and then take
care of communicating between the module and the vmm. For simplicity,
we'll make it so when a special file associated with this module is
written to, the vmm gets poked.

```c
/*- filter TaskContent("vm-module-poke-c", TaskContentType.COMPLETED, completion='buildroot login') -*/
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
/*- endfilter -*/
```

3.  And a makefile in `modules/poke/Makefile`:

```make
/*- filter TaskContent("vm-module-poke-make", TaskContentType.COMPLETED, completion='buildroot login') -*/
obj-m += poke.o

all:
/*?tab-?*/make -C $(KHEAD) M=$(PWD) modules

clean:
/*?tab-?*/make -C $(KHEAD) M=$(PWD) clean
/*- endfilter -*/
```

4. Create a CMakeLists.txt file to define our linux module. We will again be compiling our module
as an external CMake project. We will import a special helper file (`linux-module-helpers.cmake`)
from the `vm-linux` project to help us define our Linux module. Create the following at
`modules/CMakeLists.txt`:

```cmake
/*- filter TaskContent("vm-module-poke-cmake", TaskContentType.COMPLETED, completion='buildroot login') -*/
cmake_minimum_required(VERSION 3.8.2)

if(NOT MODULE_HELPERS_FILE)
    message(FATAL_ERROR "MODULE_HELPERS_FILE is not defined")
endif()

include("${MODULE_HELPERS_FILE}")

DefineLinuxModule(poke)
/*- endfilter -*/
```

5. Update our vm apps CMakeList file to declare our poke module as an
external project and add it to our overlay. Add the following:

At the top of the file include our linux helpers:

```cmake
/*- filter TaskContent("vm-cmake-poke", TaskContentType.COMPLETED, subtask='includes', completion='buildroot login') -*/
include("../projects/camkes/vm-linux/linux-source-helpers.cmake")
/*- endfilter -*/
```

Below our includes we can add:
```cmake
/*- filter TaskContent("vm-cmake-poke", TaskContentType.COMPLETED, subtask='module', completion='buildroot login') -*/
# Setup Linux Sources
GetDefaultLinuxMajor(linux_major)
GetDefaultLinuxMinor(linux_minor)
GetDefaultLinuxMd5(linux_md5)
# Download and Configure our Linux sources
DownloadLinux(${linux_major} ${linux_minor} ${linux_md5} vm_linux_extract_dir download_vm_linux)
set(linux_config "${CMAKE_CURRENT_SOURCE_DIR}/../projects/camkes/vm-linux/linux_configs/${linux_major}.${linux_minor}/config")
set(linux_symvers "${CMAKE_CURRENT_SOURCE_DIR}/../projects/camkes/vm-linux/linux_configs/${linux_major}.${linux_minor}/Module.symvers")
ConfigureLinux(${vm_linux_extract_dir} ${linux_config} ${linux_symvers} configure_vm_linux
    DEPENDS download_vm_linux
)
# Add the external poke module project
ExternalProject_Add(poke-module
    URL file:///${CMAKE_CURRENT_SOURCE_DIR}/modules
    BINARY_DIR ${CMAKE_BINARY_DIR}/poke-module
    BUILD_ALWAYS ON
    STAMP_DIR ${CMAKE_CURRENT_BINARY_DIR}/poke-module
    EXCLUDE_FROM_ALL
    INSTALL_COMMAND ""
    DEPENDS download_vm_linux configure_vm_linux
    CMAKE_ARGS
        -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
        -DCMAKE_TOOLCHAIN_FILE=${LINUX_32BIT_TOOLCHAIN}
        -DLINUX_KERNEL_DIR=${vm_linux_extract_dir}
        -DMODULE_HELPERS_FILE=${CMAKE_CURRENT_SOURCE_DIR}/../projects/camkes/vm-linux/linux-module-helpers.cmake
)
# Add our module binary to the overlay
AddExternalProjFilesToOverlay(poke-module ${CMAKE_BINARY_DIR}/poke-module vm-overlay "lib/modules/4.8.16/kernel/drivers/vmm"
    FILES poke.ko)
/*- endfilter -*/
```

6.  We want the new module to be loaded during initialization. To do this we can write our own custom init script, create a file called `init` in
our tutorial directory with the following:

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
/*- endfilter -*/
```

Note that in the `init` script we load our new `poke` module.

7. Update our apps CMakeList file to add our init script to the
overlay. After our call to `AddExternalProjFilesToOverlay` for the poke module we can add:

```cmake
/*- filter TaskContent("vm-cmake-poke", TaskContentType.COMPLETED, subtask='init_overlay', completion='buildroot login') -*/
AddFileToOverlayDir("init" ${CMAKE_CURRENT_LIST_DIR}/init "." vm-overlay)
/*- endfilter -*/
```

and give the script executable permissions:

```bash
chmod +x init
```

8.  Rebuild the app:

/*? macros.ninja_block() ?*/

9.  Run the app:

```
Welcome to Buildroot
buildroot login: root
Password:
# grep poke /proc/devices        # figure out the major number of our driver
244 poke
# mknod /dev/poke c 244 0        # create the special file
# echo > /dev/poke               # write to the file
[ 57.389643] hi
-sh: write error: Bad address    # the shell complains, but our module is being invoked!
```

**Extra Exercise: Let's make it talk to the vmm**.

7.  In modules/poke/poke.c, replace `printk("hi\n");` with `kvm_hypercall1(4, 0);`.
The choice of 4 is because 0..3 are taken by other hypercalls.

8.  Now register a handler for this hypercall in `projects/camkes/vm/components/Init/src/main.c`:.
Add a new function at the top of the file:

```
static int poke_handler(vmm_vcpu_t *vmm_vcpu) {
    printf("POKE!!!n");
    return 0;
}
```

In the function main_continued register \`poke_handler\`:

```
reg_new_handler(&vmm, poke_handler, 4); // <--- added

/* Now go run the event loop */
vmm_run(&vmm);
```

9.  Finally re-run build-rootfs, make, and run:

```
Welcome to Buildroot
buildroot login: root
Password:
# mknod /dev/poke c 244 0
# echo > /dev/poke
POKE!!!
```

/*- filter ExcludeDocs() -*/

```cmake
/*- filter File("CMakeLists.txt") -*/
cmake_minimum_required(VERSION 3.8.2)

project(vm-app)

ImportCamkesVM()

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


/*- endfilter -*/
