<!--
     Copyright 2017, Data61, CSIRO (ABN 41 687 119 230).

     SPDX-License-Identifier: CC-BY-SA-4.0
-->

# seL4 Tutorials

Various tutorials for using seL4, its libraries, and CAmkES.

## Prerequisites

Follow the instructions for setting up your host environment on the [seL4 docsite](https://docs.sel4.systems/HostDependencies).

## Starting a tutorial

This tutorial repository is part of a larger collection of repositories, which
are required to run the tutorial and are coordinated in a manifest file. See
[this guide](https://docs.sel4.systems/Tutorials/#the-tutorials) on how to check
out a consistent set.

Once you have that, a tutorial is started through the use of the `init` script
that is provided in the root directory. Using this script you can specify a
tutorial and target machine and it will create a copy of the tutorial for you to
work on.

Example:

```sh
mkdir build_hello_world
cd build_hello_world
../init --plat pc99 --tut hello-world
```

The `init` script will initialize a build directory in the current directory and at the end
it will print out a list of files that need to be modified to complete the tutorial. Building
is performed simply be invoking `ninja`, and once the tutorial compiles it can be tested
in Qemu by using the provided simulation script through `./simulate`

## Tutorials and targets

The `-h` switch to the `init` script provides a list of different tutorials and targets that
can be provided to `--plat` and `--tut` respectively.

Most tutorials support any target platform, with the exception of hello-camkes-timer, which only
supports the zynq7000 platform.

### Virtual Machine Image

You can also download a [VirtualBox virtual machine appliance](https://trustworthy.systems/Downloads/sel4_tut_v3_lubuntu_16_041-v2.ova)([md5](https://trustworthy.systems/Downloads/sel4_tut_v3_lubuntu_16_041-v2.md5)) (3GB, based on Lubuntu 16.04.1 with all the seL4 tutorial prerequisites installed).

This appliance is based on [VirtualBox 5.1.2](https://www.virtualbox.org/wiki/Downloads).
You may also need to install the appropriate VirtualBox extensions available from the same page.

## Solutions

To view the solutions for a tutorial instead of performing the tutorial pass the `--solution` flag
to the `init` script

Example:

```sh
mkdir build_hello_world
cd build_hello_world
../init --plat pc99 --tut hello-world --solution
```

After which it will tell you where the solution files are that you can look at. You can then
do `ninja && ./simulate` to build and run the solution.

### Reporting issues or bugs in the tutorials:

Please report any issues you find in the tutorials (bugs, outdated API calls, etc) by filing an issue on the public github repository:
<https://github.com/seL4/sel4-tutorials/issues/>

### Build system tutorial

Due to custom written additions to the build system specifically for the tutorials they are
not appropriate for learning how to create and structure new applications/systems. Future
tutorials for this will be forthcoming. For now it is suggested to look at other existing
applications for ideas.

# Documentation

## Developer wiki

A walkthrough of each tutorial is available on the [`docs site`](https://docs.sel4.systems/Tutorials)

## Tutorial Slides

The slides used for the tutorial are available in [`docs`](docs).

## seL4 Manual

The seL4 manual lives in the kernel source in the [`manual`](https://github.com/seL4/seL4/tree/master/manual) directory.
To generate a PDF go into that directory and type `make`.
You will need to have LaTeX installed to build it.

A pre-generated PDF version can be found [`here`](http://sel4.systems/Info/Docs/seL4-manual-latest.pdf).

## CAmkES Documentation

CAmkES documentation lives in the camkes-tool repository in [docs/index.md](https://github.com/seL4/camkes-tool/blob/master/docs/index.md).
