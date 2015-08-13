# sel4-tutorials
Various tutorials for using seL4, its libraries, and tools.

## Prerequisites
Download the prerequisites for building and running the tutorial code following the [instructions](Prerequisites.md).

### Virtual Machine Image
You can also download a [VirtualBox virtual machine appliance](https://www.dropbox.com/s/w22i3mv2cnl8106/sel4-tutorial-lubuntu-15.04-v1.ova?dl=0) (2.6GB, based on Lubuntu 15.04 and using [VirtualBox 5.0.0](https://www.virtualbox.org/wiki/Downloads)) with all the prerequisites installed.

## Tutorial Code
The tutorial exercises are available in `apps/`.
This directory contains partially completed code and comments on how to complete it.  Sample solutions are available in `solutions/`.

To get a copy of the seL4 API and library exercise code and all libraries and tools needed to build and run it do the following:
    mkdir sel4-tutorials
    cd sel4-tutorials
    repo init -u http://github.com/sel4-projects/sel4-tutorials-manifest
    repo sync -m sel4-tutorials.xml

To get a copy of the CAmkES exercise code and all libraries and tools needed to build and run it do the following:
    mkdir camkes-tutorials
    cd camkes-tutorials
    repo init -u http://github.com/sel4-projects/sel4-tutorials-manifest
    repo sync -m camkes-tutorials.xml
