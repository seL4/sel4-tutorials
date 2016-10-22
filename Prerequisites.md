# Prerequisites

Before building and running seL4-based systems you must have several prerequisite software packages installed on the machine you will be using to build.  

## Linux

First of all we assume that you are running Linux.  We typically use some flavour of Ubuntu or other Debian-based system.  You could try using other Unix systems (including Mac OS) but your mileage may vary.  The following instructions assume Ubuntu version 14.04 (Trusty Tahr) or greater on a 64-bit machine.

# Quick-Start

    sudo add-apt-repository universe
    sudo apt-get install -y python-software-properties
    sudo apt-get update

    sudo apt-get install -y build-essential realpath
    sudo apt-get install -y gcc-multilib ccache ncurses-dev
    sudo apt-get install -y gcc-arm-linux-gnueabi
    sudo apt-get install -y gcc-arm-none-eabi

    sudo apt-get install -y python-pip python-jinja2 python-ply  python-tempita
    sudo pip install --upgrade pip
    sudo pip install pyelftools

    sudo add-apt-repository -y ppa:hvr/ghc
    sudo apt-get update
    sudo apt-get install -y ghc-7.8.4 cabal-install-1.22

    echo export PATH=/opt/ghc/7.8.4/bin:/opt/cabal/1.22/bin:\$PATH >> ~/.bashrc
    export PATH=/opt/ghc/7.8.4/bin:/opt/cabal/1.22/bin:$PATH
 
    cabal update
    cabal install base-compat-0.9.0
    cabal install MissingH-1.3.0.2
    cabal install data-ordlist
    cabal install split
    cabal install mtl

    sudo apt-get install -y qemu-system-arm qemu-system-x86

    sudo apt-get install -y libxml2-utils

    sudo apt-get install -y git phablet-tools

# Test It

Test that it works by getting seL4 and CAmkES and building and running it.

Get seL4 and CAmkES

    mkdir camkes-manifest
    cd camkes-manifest
    repo init -u https://github.com/sel4/camkes-manifest.git
    repo sync

Test it for ARM

    make arm_simple_defconfig; make silentoldconfig
    make
    qemu-system-arm -M kzm -nographic -kernel images/capdl-loader-experimental-image-arm-imx31
    # quit with Ctrl-A X

Test it for x86

    make clean
    make ia32_simple_defconfig; make silentoldconfig
    make
    qemu-system-i386 -nographic -m 512 -kernel images/kernel-ia32-pc99 -initrd images/capdl-loader-experimental-image-ia32-pc99
    # quit with Ctrl-A X

# More Detailed Explanation

The following is a more detailed explanation of the prerequisites.  You don?t need to execute the commands here if you?ve already done the Quick-Start ones above.

## Toolchains

Building and running seL4 and CAmkES reguires: 
 * an appropriate (cross) compiler and build tools
 * Python and Python packages (Jinja2, PLY, pyelftools)
 * Haskell and Cabal (a Haskell package manager) 
 * Haskell packages (MissingH, Data.Ordlist, Split).
 * qemu (a CPU emulator)
 * git and repo
 * various other libraries

### Cross Compiler and Build Tools

    sudo apt-get update
    sudo apt-get install build-essential realpath 
    sudo apt-get install gcc-multilib ccache ncurses-dev
    sudo add-apt-repository universe
    sudo apt-get update
    sudo apt-get install gcc-arm-linux-gnueabi
    sudo apt-get install gcc-arm-none-eabi


### Python and Python packages

    sudo apt-get install python-software-properties
    sudo apt-get update
    sudo apt-get install python-pip python-jinja2 python-ply  python-tempita
    sudo pip install --upgrade pip
    sudo pip install pyelftools
    # if pip can't be found try doing `hash -d pip`

### Haskell, Cabal and Cabal Packages

We need a newer version of Haskell than available from Ubuntu, so we specify the exact version of Hsakell and Cabal that we need.
These don't get installed in the default spot, so add them to the path too.

    sudo add-apt-repository -y ppa:hvr/ghc
    sudo apt-get update
    sudo apt-get install -y ghc-7.8.4 cabal-install-1.22

    echo export PATH=/opt/ghc/7.8.4/bin:/opt/cabal/1.22/bin:\$PATH >> ~/.bashrc
    export PATH=/opt/ghc/7.8.4/bin:/opt/cabal/1.22/bin:$PATH
 
    cabal update
    cabal install base-compat-0.9.0
    cabal install MissingH-1.3.0.2
    cabal install data-ordlist
    cabal install split
    cabal install mtl

### qemu

    sudo apt-get install qemu-system-arm qemu-system-x86

### git and repo

    sudo apt-get install git phablet-tools

### various other libraries

    sudo apt-get install libxml2-utils
	

