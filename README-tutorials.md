# sel4-tutorials
Various tutorials for using seL4, its libraries, and CAmkES.

## Prerequisites
Download the prerequisites for building and running the tutorial code following the [instructions](Prerequisites.md).

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

## Tutorial Slides
The slides used for the tutorial are available in [`docs`](docs).

## seL4 Manual
The seL4 manual lives in the kernel source in the [`manual`](https://github.com/seL4/seL4/tree/master/manual) directory.
To generate a PDF go into that directory and type `make`.
You will need to have LaTeX installed to build it.

We've also included a pre-generated PDF version in [`docs/manual.pdf'](docs/manual.pdf)

## CAmkES Documentation
CAmkES documentation lives in the camkes-tool repository in [docs/index.md](https://github.com/seL4/camkes-tool/blob/master/docs/index.md).
