#
# Copyright 2014, NICTA
#
# This software may be distributed and modified according to the terms of
# the BSD 2-Clause license. Note that NO WARRANTY is provided.
# See "LICENSE_BSD2.txt" for details.
#
# @TAG(NICTA_BSD)
#

lib-dirs:=libs

# The main target we want to generate
all: app-images

-include .config

include tools/common/project.mk

# Some example qemu invocations

# note: this relies on qemu after version 2.0
simulate-kzm:
	qemu-system-arm -nographic -M kzm \
		-kernel images/sel4test-driver-image-arm-imx31

# This relies on a helper script to build a bootable image
simulate-beagle:
	beagle_run_elf images/sel4test-driver-image-arm-omap3

simulate-ia32:
	qemu-system-i386 \
		-m 512 -nographic -kernel images/kernel-ia32-pc99 \
		-initrd images/sel4test-driver-image-ia32-pc99

.PHONY: help
help:
	@echo "sel4test - unit and regression tests for seL4"
	@echo " make menuconfig      - Select build configuration via menus."
	@echo " make <defconfig>     - Apply one of the default configurations. See"
	@echo "                        below for valid configurations."
	@echo " make silentoldconfig - Update configuration with the defaults of any"
	@echo "                        newly introduced settings."
	@echo " make                 - Build with the current configuration."
	@echo ""
	@echo "Valid default configurations are:"
	@ls -1 configs | sed -e 's/\(.*\)/\t\1/g'
