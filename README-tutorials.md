<!--
     Copyright 2017, Data61
     Commonwealth Scientific and Industrial Research Organisation (CSIRO)
     ABN 41 687 119 230.

     This software may be distributed and modified according to the terms of
     the BSD 2-Clause license. Note that NO WARRANTY is provided.
     See "LICENSE_BSD2.txt" for details.

     @TAG(DATA61_BSD)
-->
# sel4-tutorials
Various tutorials for using seL4, its libraries, and CAmkES.

## Prerequisites
Download the prerequisites for building and running the tutorial code following the [instructions](Prerequisites.md).

## Building using CMake:

### Caveats:

#### No support for CAmkES currently:

Only the native seL4 tutorials can currently be built with the CMake build
system. The CAmkES tutorials are currently unsupported. I.e, the following
tutorials cannot be built with the CMake build system:

* hello-camkes-0
* hello-camkes-1
* hello-camkes-2
* hello-camkes-timer

### Choosing your target machine and compiling

If you're compiling for **any** ARM platform, please don't forget to pass in the (required)
extra `-DAARCH32=1` or `-DAARCH64=1` flag (depending on whether you're compiling for
32-bit or 64-bit ARM) when invoking CMake.

Invoke the CMake build system to begin trying your changes to the exercises:

    # Create a new folder in which to compile your files:
    mkdir cbuild
    cd cbuild

    # Invoke CMake using the shell script wrapper in the root directory:
     ../init-build.sh \
        -DTUTORIAL=<PREFERENCE> \
        -DTUT_BOARD=<PREFERENCE> \
        -DTUT_ARCH=<PREFERENCE> \
        -DBUILD_SOLUTIONS=<BOOLEAN_PREFERENCE>

    # Invoke ninja to compile the tutorials
    ninja

* `TUTORIAL` is used to select the tutorial you'd like to compile.
* `BUILD_SOLUTIONS` can be set to true or false to determine whether the exercises or the solutions are selected for compilation.

Use combinations of `TUT_BOARD` and `TUT_ARCH` to select the target machine to
compile for.

The *officially* supported combinations of `TUT_BOARD` and `TUT_ARCH` are:

* `TUT_BOARD`=pc, `TUT_ARCH`=x86: This yields an x86-pc build.
* `TUT_BOARD`=pc, `TUT_ARCH`=x86_64: This yields an x86_64-pc build.
* `TUT_BOARD`=zynq7000: This yields an armv7-zynq7000 build (don't forget to add `-DAARCH32=1`)

Other boards may work, and the hello-1, hello-2, hello-3 and hello-4 tutorials
should work on most boards. Hello-timer may not work on many boards.

After selecting your target machine and tutorial please type `ninja` to compile
and then look for your compiled images inside of the subfolder `images` within
your build directory.

Power up your favourite emulator or transfer the images to your target board,
and enjoy!

### Virtual Machine Image
You can also download a [VirtualBox virtual machine appliance](http://ts.data61.csiro.au/Downloads/sel4_tut_v3_lubuntu_16_041-v2.ova)([md5](http://ts.data61.csiro.au/Downloads/sel4_tut_v3_lubuntu_16_041-v2.md5)) (3GB, based on Lubuntu 16.04.1 with all the seL4 tutorial prerequisites installed).

This appliance is based on [VirtualBox 5.1.2](https://www.virtualbox.org/wiki/Downloads).  
You may also need to install the appropriate [VirtualBox extensions](http://download.virtualbox.org/virtualbox/5.1.2/Oracle_VM_VirtualBox_Extension_Pack-5.1.2-108956.vbox-extpack).

## Tutorial Code
The tutorial exercises are available in [`apps`](apps).
This directory contains partially completed code and comments on how to complete it.  Sample solutions are available in [`solutions`](solutions).

To get a copy of the seL4 API and library exercise code and all libraries and tools needed to build and run it do the following:

        mkdir sel4-tutorials
        cd sel4-tutorials
        repo init -u http://github.com/SEL4PROJ/sel4-tutorials-manifest -m sel4-tutorials.xml
        repo sync

To get a copy of the CAmkES exercise code and all libraries and tools needed to build and run it do the following:

        mkdir camkes-tutorials
        cd camkes-tutorials
        repo init -u http://github.com/SEL4PROJ/sel4-tutorials-manifest -m camkes-tutorials.xml
        repo sync

### Reporting issues or bugs in the tutorials:
Please report any issues you find in the tutorials (bugs, outdated API calls, etc) by filing an issue on the public github repository:
https://github.com/SEL4PROJ/sel4-tutorials/issues

# Documentation

## Developer wiki
A walkthrough of each tutorial is available on the [`wiki`](https://wiki.sel4.systems/Tutorials)

## Tutorial Slides
The slides used for the tutorial are available in [`docs`](docs).

## seL4 Manual
The seL4 manual lives in the kernel source in the [`manual`](https://github.com/seL4/seL4/tree/master/manual) directory.
To generate a PDF go into that directory and type `make`.
You will need to have LaTeX installed to build it.

We've also included a pre-generated PDF version in [`docs/manual.pdf'](docs/manual.pdf)

## CAmkES Documentation
CAmkES documentation lives in the camkes-tool repository in [docs/index.md](https://github.com/seL4/camkes-tool/blob/master/docs/index.md).
