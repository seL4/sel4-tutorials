# sel4-tutorials
Various tutorials for using seL4, its libraries, and CAmkES.

## Prerequisites
Download the prerequisites for building and running the tutorial code following the [instructions](Prerequisites.md).

### Virtual Machine Image
You can also download a [VirtualBox virtual machine appliance](http://ssrg.nicta.com.au/Downloads/sel4-tutorial-lubuntu-15.04-v4.ova)([md5](http://ssrg.nicta.com.au/Downloads/sel4-tutorial-lubuntu-15.04-v4.md5)) (3.4GB, based on Lubuntu 15.04 with all the seL4 tutorial prerequisites installed).

This appliance is based on [VirtualBox 5.0.4](https://www.virtualbox.org/wiki/Downloads).  
You may also need to install the appropriate [VirtualBox extensions](http://download.virtualbox.org/virtualbox/5.0.4/Oracle_VM_VirtualBox_Extension_Pack-5.0.4-102546.vbox-extpack).

## Tutorial Code
The tutorial exercises are available in [`apps`](apps).
This directory contains partially completed code and comments on how to complete it.  Sample solutions are available in [`solutions`](solutions).

To get a copy of the seL4 API and library exercise code and all libraries and tools needed to build and run it do the following:

        mkdir sel4-tutorials
        cd sel4-tutorials
        repo init -u http://github.com/sel4-projects/sel4-tutorials-manifest -m sel4-tutorials.xml
        repo sync 

To get a copy of the CAmkES exercise code and all libraries and tools needed to build and run it do the following:

        mkdir camkes-tutorials
        cd camkes-tutorials
        repo init -u http://github.com/sel4-projects/sel4-tutorials-manifest -m camkes-tutorials.xml
        repo sync 

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
