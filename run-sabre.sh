#! /bin/bash

set -e

APP=$1

make sabre_${APP}_defconfig
make silentoldconfig
make
if [[ ${APP} =~ .*camkes.* ]]; then
    qemu-system-arm -M sabrelite -nographic -kernel   images/capdl-loader-experimental-image-arm-imx6
else
    qemu-system-arm -M sabrelite -nographic -machine sabrelite -m size=1024M -serial null -serial mon:stdio -kernel   images/${APP}-image-arm-imx6
fi

