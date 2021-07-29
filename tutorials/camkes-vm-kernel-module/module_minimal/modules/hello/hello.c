/*
 * Copyright 2021 Michael Neises
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <asm/uaccess.h>
#include <asm/kvm_para.h>
#include <asm/io.h>

static int __init hello_init(void)
{
    printk("\n==============================\n");
    printk("Greetings");
    printk("\n==============================\n");
    return 0;
}

static void __exit hello_exit(void)
{
    printk("\n==============================\n");
    printk("Bye-Bye");
    printk("\n==============================\n");
}

module_init(hello_init);
module_exit(hello_exit);