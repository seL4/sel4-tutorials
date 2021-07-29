<!--
  Copyright 2021 Michael Neises

  SPDX-License-Identifier: BSD-2-Clause
-->
# Cross-Compiling Kernel Modules for use with seL4's Linux VM using qemu-arm-virt

This is a procedure to prepare a camkes app for cross-compiling linux kernel modules for use in virtualization.

See [the camkes app](https://github.com/seL4/sel4-tutorials/tree/master/tutorials/camkes-vm-kernel-module/module_minimal) for a ready-to-build project.

See [an automation](https://github.com/NeisesResearch/kernel_module_workstation) of this procedure. Cloning that repo and running the setup script there should give you a camkes application, ready to build and simulate, which demonstrates the cross-compilation of a trivial kernel module.

1. download linux kernel repo
```
git clone git://git.kernel.org/pub/scm/linux/kernel/git/stable/linux-stable.git
cd linux-stable
git checkout linux-4.9.y
git fetch
```

2. grab the config from seL4
```
make clean
rm Module.symvers
rm .config
rm .config.old
cp .../projects/camkes-vm-images/qemu-arm-virt/linux_configs/config ./.config
```

3. prepare kernel for cross-compilation, choosing default config options always
```
make ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- prepare
```

4. build linux kernel (where X is the number of tasks you want to create)
```
make -jX ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu-
```

5. add the newly built kernel to the camkes build path
```
cp arch/arm64/boot/Image [...]/projects/camkes-vm-images/qemu-arm-virt/newLinux
```

6. edit your cmakelists.txt to use the new kernel by replacing this line:
```
AddToFileServer("linux" "${CAMKES_VM_IMAGES_DIR}/qemu-arm-virt/linux")
```
with this line:
```
AddToFileServer("linux" "${CAMKES_VM_IMAGES_DIR}/qemu-arm-virt/newLinux")
```
7. from the kernel build directory, add the config and symbols files to the camkes build path
```
mkdir -p [...]/projects/camkes-vm-linux/linux_configs/4.9.y/64
cp .config [...]/projects/camkes-vm-linux/linux_configs/4.9.y/64/config
cp Module.symvers [...]/projects/camkes-vm-linux/linux_configs/4.9.y/64/Module.symvers
```

8. update CMakeLists.txt to build the module using the new kernel sources
```
# Setup and Configure Linux Sources
set(linux_config "${CAMKES_VM_LINUX_DIR}/linux_configs/4.9.y/64/config")
set(linux_symvers "${CAMKES_VM_LINUX_DIR}/linux_configs/4.9.y/64/Module.symvers")

# linux_dir should point to wherever you built the kernel
set(linux_dir "/host/linux-stable")

ConfigureLinux(${linux_dir} ${linux_config} ${linux_symvers} configure_vm_linux)

# Add the external hello module project
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
AddExternalProjFilesToOverlay(hello-module
${CMAKE_CURRENT_BINARY_DIR}/hello-module vm-overlay "lib/modules/4.9.275/kernel/drivers/vmm"
    FILES hello.ko)

AddFileToOverlayDir("init" ${CMAKE_CURRENT_SOURCE_DIR}/init "/etc/init.d" vm-overlay)

# Generate overlayed rootfs

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

9. Ensure the module Makefile is configured for cross compilation
```
obj-m += hello.o

all:
	make ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- -C $(KHEAD) M=$(PWD) modules

clean:
	make ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- -C $(KHEAD) M=$(PWD) clean

```

10. Build your [project](https://github.com/seL4/sel4-tutorials/tree/master/tutorials/camkes-vm-kernel-module/module_minimal) like normal.
