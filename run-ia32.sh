#! /bin/bash

set -e

APP=$1

make ia32_${APP}_defconfig
make silentoldconfig
make
if [[ ${APP} =~ .*camkes.* ]]; then
    qemu-system-i386 -nographic -m 512   -kernel images/kernel-ia32-pc99   -initrd images/capdl-loader-experimental-image-ia32-pc99 
else
    qemu-system-i386 -nographic -m 512   -kernel images/kernel-ia32-pc99   -initrd images/${APP}-image-ia32-pc99 
fi


