<!--
  Copyright 2021 Michael Neises

  SPDX-License-Identifier: BSD-2-Clause
-->
# Cross-Compiling Kernel Modules for use with seL4's Linux VM using qemu-arm-virt

This is a procedure to prepare a CAmkES app for cross-compiling linux kernel modules for use in virtualization.

See the "module_minimal" app for a ready-to-build CAmkES application.

1. Download linux kernel repository.
```
git clone git://git.kernel.org/pub/scm/linux/kernel/git/stable/linux-stable.git
cd linux-stable
git checkout linux-4.9.y
git fetch
```
In this step we gather the linux kernel source for compilation later. We require the linux source in order to compile kernel modules against it. If we have a version mismatch, or even a configuration mismatch at compile-time, our kernel modules will not be considered "valid" at run-time. We choose to use linux-4.9.y (the latest version of 4.9) because seL4 supports a build-configuration for this version of linux, which we will use in the next step.

2. Borrow the linux-kernel configuration from seL4.
```
make clean
rm Module.symvers
rm .config
rm .config.old
cp .../projects/camkes-vm-images/qemu-arm-virt/linux_configs/config ./.config
```
In this step we copy the particular configuration used to compile the linux kernel for seL4. There are myriad configuration options available, so this is extremely convenient.

3. Prepare kernel for cross-compilation, choosing default config options always.
```
make ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- prepare
```
In this step we perform the "first half" of the kernel compilation. This configures our build system using the configuration we borrowed from seL4 in the last step. There are a few options apparently not disambiguated by the configuration we borrowed, so we have to manually accept a few options. The default choices for these options are acceptable, so at this step I literally press my "Enter" button three times fast.

4. Build linux kernel (where X is the number of tasks you want to create).
```
make -jX ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu-
```
In this step we perform the "second half" of compilation. Namely, we actually compile the kernel, which might take hours depending on your machine. If you have the processor cores to spare, I highly recommend setting X in this invocation to as high a number as your machine can muster. The linux kernel compilation process appears very highly parallelized, so when I set X to be `$(nproc)`, which is "the number of available processing units," I can garner a speedup of nearly 6. In a concrete sense, my machine takes about 90 minutes in the single-core case, and only about 15 minutes in the `$(nproc)`-core case.

5. Add the newly built kernel to the CAmkES build path.
```
cp arch/arm64/boot/Image [...]/projects/camkes-vm-images/qemu-arm-virt/newLinux
```
In this step we place our newly built kernel into a natural and convenient place in the CAmkES filepath. This is so we can access it easily in the next step.

6. Edit your CAmkES application's root cmakelists.txt to use the new kernel by replacing this line:
```
AddToFileServer("linux" "${CAMKES_VM_IMAGES_DIR}/qemu-arm-virt/linux")
```
with this line:
```
AddToFileServer("linux" "${CAMKES_VM_IMAGES_DIR}/qemu-arm-virt/newLinux")
```
In this step we redirect the CMake build system away from using the provided linux kernel and towards using our newly built kernel. That is, seL4 comes pre-packaged with a Linux kernel ready-to-use, but this ready-to-use kernel is not sufficient for our kernel-module purposes. For this reason, we must ensure our application uses our newly built kernel instead.

7. From the kernel build directory, add the config and symbols files to the CAmkES build path.
```
mkdir -p [...]/projects/camkes-vm-linux/linux_configs/4.9.y/64
cp .config [...]/projects/camkes-vm-linux/linux_configs/4.9.y/64/config
cp Module.symvers [...]/projects/camkes-vm-linux/linux_configs/4.9.y/64/Module.symvers
```
In this step, we add information regarding the compilation of our kernel to the CAmkES filepath. We are required to do this in order for the kernel-module build to succeed in a later step. Namely, our kernel modules must know two things in order to be compatible with the kernel: how the kernel was built and what symbols its build process exported.

8. Update CMakeLists.txt to build the module using the new kernel sources.
```
# Setup and Configure Linux Sources
set(linux_config "${CAMKES_VM_LINUX_DIR}/linux_configs/4.9.y/64/config")
set(linux_symvers "${CAMKES_VM_LINUX_DIR}/linux_configs/4.9.y/64/Module.symvers")

# linux_dir should point to wherever you built the kernel
set(linux_dir "/host/linux-stable")

ConfigureLinux(${linux_dir} ${linux_config} ${linux_symvers} configure_vm_linux)

# Setup our kernel module for compilation as part of the standard CAmkES-app build process.
ExternalProject_Add(hello-module
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/modules
    BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/hello-module
    BUILD_ALWAYS ON
    STAMP_DIR ${CMAKE_CURRENT_BINARY_DIR}/hello-module-stamp
    EXCLUDE_FROM_ALL
    INSTALL_COMMAND ""
    DEPENDS configure_vm_linux
    CMAKE_ARGS
        -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
        -DCMAKE_TOOLCHAIN_FILE=${LINUX_64BIT_TOOLCHAIN}
        -DLINUX_KERNEL_DIR=${linux_dir}
        -DMODULE_HELPERS_FILE=${CAMKES_VM_LINUX_DIR}/linux-module-helpers.cmake
)

# Add our module binary to the overlay
AddExternalProjFilesToOverlay(
    hello-module
    ${CMAKE_CURRENT_BINARY_DIR}/hello-module
    vm-overlay
    "lib/modules/4.9.275/kernel/drivers/vmm"
    FILES
    hello.ko)

# Add an init script which should load our module at boot-time
AddFileToOverlayDir("init" ${CMAKE_CURRENT_SOURCE_DIR}/init "/etc/init.d" vm-overlay)

# Add our overlay to the root filesystem used by Linux
AddOverlayDirToRootfs(
    vm-overlay
    ${rootfs_file}
    "buildroot"
    "rootfs_install"
    output_overlayed_rootfs_location
    rootfs_target
    GZIP
)
AddToFileServer("linux-initrd" ${output_overlayed_rootfs_location} DEPENDS rootfs_target)
```
In this step we equip CMake to handle our kernel module compilation and addition to the Linux file system.

9. Ensure the kernel-module's Makefile is configured for cross compilation.
```
obj-m += hello.o

all:
	make ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- -C $(KHEAD) M=$(PWD) modules

clean:
	make ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- -C $(KHEAD) M=$(PWD) clean

```

10. Build your CAmkES application like normal.
