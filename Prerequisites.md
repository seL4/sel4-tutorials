# Prerequisites

Before building and running seL4-based systems you must have several prerequisite software packages installed on the machine you will be using to build.  

## Linux

First of all we assume that you are running Linux.  We typically use some flavour of Ubuntu or other Debian-based system.  You could try using other Unix systems (including Mac OS) but your mileage may vary.  The following instructions assume Ubuntu version 14.04 (Trusty Tahr) or greater.

# Quick-Start

    sudo add-apt-repository universe
    sudo apt-get install -y python-software-properties
    sudo apt-get update

    sudo apt-get install -y build-essential realpath 
    sudo apt-get install -y gcc-multilib ccache ncurses-dev
    sudo apt-get install -y gcc-arm-linux-gnueabi

    sudo apt-get install -y python-pip python-jinja2 python-ply  python-tempita
    sudo pip install --upgrade pip
    hash -d pip
    sudo pip install pyelftools

    sudo apt-get install -ycabal-install ghc libghc-missingh-dev libghc-split-dev
    cabal update
    cabal install data-ordlist

    sudo apt-get install -y qemu-system-arm qemu-system-x86

    sudo apt-get install -y realpath libxml2-utils
    sudo apt-get install -y lib32z1 lib32bz2-1.0 lib32ncurses5

    sudo apt-get install -y git phablet-tools

    git config --global user.email "<email>"

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


### Python and Python packages

    sudo apt-get install python-software-properties
    sudo apt-get update
    sudo apt-get install python-pip python-jinja2 python-ply  python-tempita
    sudo pip install --upgrade pip
    hash -d pip
    sudo pip install pyelftools

### Haskell, Cabal and Cabal Packages

    sudo apt-get install cabal-install ghc libghc-missingh-dev libghc-split-dev
    cabal update
    cabal install data-ordlist

### qemu

    sudo apt-get install qemu-system-arm qemu-system-x86

### git and repo

    sudo apt-get install git phablet-tools

    # tell git your email
    git config --global user.email "<email>"

### various other libraries

    sudo apt-get install realpath libxml2-utils
    sudo apt-get install lib32z1 lib32bz2-1.0 lib32ncurses5
	

