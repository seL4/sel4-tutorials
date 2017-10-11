<!--
     Copyright 2017, Data61
     Commonwealth Scientific and Industrial Research Organisation (CSIRO)
     ABN 41 687 119 230.

     This software may be distributed and modified according to the terms of
     the BSD 2-Clause license. Note that NO WARRANTY is provided.
     See "LICENSE_BSD2.txt" for details.

     @TAG(DATA61_BSD)
-->
# Prerequisites

Before building and running seL4-based systems you must have several prerequisite software packages installed on the machine you will be using to build.

## Linux

First of all we assume that you are running Linux.  We typically use some flavour of Ubuntu or other Debian-based system.  You could try using other Unix systems (including Mac OS) but your mileage may vary.  The following instructions assume Ubuntu version 16.04 (Xenial Xerus) or greater on a 64-bit machine. Note that the commands starting with `apt-get` must be run as root.

# Quick-Start

```bash
    apt-get install git repo cmake ninja-build clang gcc-multilib gcc-arm-none-eabi binutils-arm-none-eabi \
    libncurses-dev libxml2-utils libssl-dev libsqlite3-dev libcunit1-dev expect python-pip

    pip install camkes-deps

    # Required for CAmkES only
    curl -sSL https://get.haskellstack.org/ | sh
```

# Test It

Test that it works by getting seL4 and CAmkES and building and running it.

Get seL4 and CAmkES

```bash
    mkdir camkes-manifest
    cd camkes-manifest
    repo init -u https://github.com/sel4/camkes-manifest.git
    repo sync
```

Test it for ARM

```bash
    make arm_simple_defconfig; make silentoldconfig
    make
    qemu-system-arm -M kzm -nographic -kernel images/capdl-loader-experimental-image-arm-imx31
    # quit with Ctrl-A X
```

Test it for x86

```bash
    make clean
    make x86_simple_defconfig; make silentoldconfig
    make
    qemu-system-i386 -nographic -m 512 -cpu Haswell -kernel images/kernel-ia32-pc99 -initrd images/capdl-loader-experimental-image-ia32-pc99
    # quit with Ctrl-A X
```

# More Detailed Explanation

The following is a more detailed explanation of the prerequisites.  You don't need to execute the commands here if you've already done the Quick-Start ones above.

## Toolchains

Building and running seL4 and CAmkES reguires:
 * git and repo
 * cross compiler and build tools
 * python packages
 * haskell-stack (a haskell version and package manager)
 * qemu (a CPU emulator)

### Git and Repo

```bash
    apt-get install git repo
```

### Cross Compiler and Build Tools

```bash
    # Tools for building our build tools
    apt-get install cmake ninja-build clang

    # Cross compilers
    apt-get install gcc-multilib gcc-arm-none-eabi binutils-arm-none-eabi

    # Libraries and tools needed by our build system
    apt-get install libncurses-dev libxml2-utils libssl-dev libsqlite3-dev libcunit1-dev expect
```

### Python Packages

```bash
    apt-get install python-pip
    pip install camkes-deps
```

### Haskell Stack (required for CAmkES only)

Haskell stack is a tool for managing ghc versions and haskell packages.
Learn more on their [website](https://haskellstack.org).

```bash
    curl -sSL https://get.haskellstack.org/ | sh
```

### Qemu

```bash
    apt-get install qemu-system-arm qemu-system-x86
```
