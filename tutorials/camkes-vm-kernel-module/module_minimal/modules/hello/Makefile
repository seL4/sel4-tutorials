#  Copyright 2021 Michael Neises
#
#  SPDX-License-Identifier: BSD-2-Clause
#
obj-m += hello.o
all:
	make ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- -C $(KHEAD) M=$(PWD) modules
clean:
	make ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- -C $(KHEAD) M=$(PWD) clean
