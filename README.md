<!--
     Copyright 2024, seL4 Project a Series of LF Projects, LLC..

     SPDX-License-Identifier: CC-BY-SA-4.0
-->

# seL4 Tutorials

Various tutorials for using seL4, its libraries, and CAmkES.

## Prerequisites

Follow the instructions for setting up your host environment on the [seL4
docsite](https://docs.sel4.systems/Tutorials/setting-up.html).

### Troubleshooting missing host tools

If a tutorial build fails with:

```text
xmllint: command not found
```

install the XML command-line tools package:

```sh
sudo apt install libxml2-utils
```

If a tutorial build fails while generating `capDL-tool/parse-capDL` with:

```text
stack: No such file or directory
```

install Haskell Stack:

```sh
sudo apt install haskell-stack
```

## Starting a tutorial

This tutorial repository is part of a larger collection of repositories, which
are required to run the tutorial and are coordinated in a manifest file. After
the setup steps above, see [this
guide](https://docs.sel4.systems/Tutorials/get-the-tutorials.html) on how to
check out a consistent set.

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

## Documentation

### seL4 docsite

A walkthrough of each tutorial is available on the [seL4 docsite](https://docs.sel4.systems/Tutorials/)

### Tutorial Slides

The slides used for the tutorial are available in [`docs`](docs/).

### seL4 Manual

A pre-generated PDF version of the seL4 manual can be found
[`here`](http://sel4.systems/Info/Docs/seL4-manual-latest.pdf).

### CAmkES Documentation

CAmkES documentation can be found on the [CAmkES page](https://docs.sel4.systems/projects/camkes/) of the docsite.
