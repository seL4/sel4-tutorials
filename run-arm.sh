#! /bin/bash

set -e

APP=$1

make arm_${APP}_defconfig
make silentoldconfig
make
if [[ ${APP} =~ .*camkes.* ]]; then
    qemu-system-arm -M kzm -nographic -kernel   images/capdl-loader-experimental-image-arm-imx31
else
    qemu-system-arm -M kzm -nographic -kernel   images/${APP}-image-arm-imx31
fi


