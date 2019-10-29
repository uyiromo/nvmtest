/*
 * The MIT License (MIT)

 * Copyright (c) 2019 Yu Omori

 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is furnis-
 * hed to do so, subject to the following conditions:

 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.

 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABI-
 * LITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT
 * OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/sched.h>
#include <linux/device.h>
#include <asm/current.h>
#include <asm/uaccess.h>
#include <linux/uaccess.h>
#include <linux/timekeeping.h>
#include <asm/io.h>

#include "wbmod.h"

MODULE_LICENSE("Dual BSD/GPL");

/* Base of minor number and Number of maximum devices*/
static const unsigned int MINOR_BASE = 0;
static const unsigned int MINOR_NUM  = 1;

/* Major number & Device Object */
static unsigned int wbmod_major;
static struct cdev wbmod_cdev;

static flush_range range;

#define _DCCMVAC(addr) \
    __asm__ __volatile__ ( \
        "MCR p15, 0, %0, c7, c10, 1\n" \
        : : "r"(addr) :)

#define _DCIMVAC(addr) \
    __asm__ __volatile__ ( \
        "MCR p15, 0, %0, c7, c6, 1\n" \
        : : "r"(addr) :)

#define _DMB()                                  \
    __asm__ __volatile__ ("DSB SY")



static int
wbmod_open(struct inode *inode, struct file *file)
{
    return 0;
}

static int
wbmod_close(struct inode *inode, struct file *file)
{
    return 0;
}

static ssize_t
wbmod_read(struct file *filp, char __user *buf, size_t count,
           loff_t *f_pos)
{
    return 1;
}

static ssize_t
wbmod_write(struct file *filp, const char __user *buf, size_t count,
            loff_t *f_pos)
{
    return 1;
}

static long
wbmod_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{

    unsigned long base, addr, size;

    switch (cmd) {
    case WBMOD_DCCMVAC:
        /* Get address from user space */
        if (get_user(addr, (unsigned long __user *)arg)) {
            printk(KERN_ALERT "Failed to get writeback addr\n");
            return -EFAULT;
        }

        _DCCMVAC(addr);

        break;
    case WBMOD_DCCMVAC_RANGE:
        /* Get address from user space */
        if (copy_from_user(&range, (void __user *) arg, sizeof(range))) {
            printk(KERN_ALERT "Failed to get writeback addr\n");
            return -EFAULT;
        }

        /* per cacheline (32-Byte) */
        base = range.va_base;
        size = range.size;

        _DMB();
        for (addr = base; addr < base + size; addr += 32) {
            _DCCMVAC(addr);
        }
        _DMB();

        break;
    case WBMOD_DCCMVAC_RANGE_RELAX:
        /* Get address from user space */
        if (copy_from_user(&range, (void __user *) arg, sizeof(range))) {
            printk(KERN_ALERT "Failed to get writeback addr\n");
            return -EFAULT;
        }

        /* per cacheline (32-Byte) */
        base = range.va_base;
        size = range.size;

        for (addr = base; addr < base + size; addr += 32) {
            _DCCMVAC(addr);
        }

        break;
    case WBMOD_DCIMVAC_RANGE:
        /* Invalidate all cache fetched from NVM address space */
        if (copy_from_user(&range, (void __user *) arg, sizeof(range))) {
            printk(KERN_ALERT "Failed to get invalidate addr\n");
            return -EFAULT;
        }

        /* per cacheline (32-Byte) */
        base = range.va_base;
        size = range.size;

        _DMB();
        for (addr = base; addr < base + size; addr += 32)
            _DCIMVAC(addr);
        _DMB();

        break;
    default:
        printk(KERN_ALERT "Given command is not supported:%d\n", cmd);
        return -EFAULT;
    }

    return 0;
}


struct file_operations wbmod_fops = {
    .open           = wbmod_open,
    .release        = wbmod_close,
    .read           = wbmod_read,
    .write          = wbmod_write,
    .unlocked_ioctl = wbmod_ioctl,
    .compat_ioctl   = wbmod_ioctl,
};

/* Entry of insmod */
static int wbmod_init(void)
{
    int alloc_ret = 0;
    int cdev_err = 0;
    dev_t dev;
    unsigned int PERIPHBASE;
    char *base;
    unsigned int *p;

    printk(KERN_ALERT "wbmod_init is called\n");

    /* Get major number dynamically */
    alloc_ret = alloc_chrdev_region(&dev, MINOR_BASE, MINOR_NUM, WBMOD_NAME);
    if (alloc_ret != 0) {
        printk(KERN_ERR  "alloc_chrdev_region = %d\n", alloc_ret);
        return -1;
    }

    /* Get major number & Create device object  */
    wbmod_major = MAJOR(dev);
    dev = MKDEV(wbmod_major, MINOR_BASE);

    /* Init cdev & Register handler table */
    cdev_init(&wbmod_cdev, &wbmod_fops);
    wbmod_cdev.owner = THIS_MODULE;

    /* 4. Register device to kernel */
    cdev_err = cdev_add(&wbmod_cdev, dev, MINOR_NUM);
    if (cdev_err != 0) {
        printk(KERN_ERR  "cdev_add = %d\n", alloc_ret);
        unregister_chrdev_region(dev, MINOR_NUM);
        return -1;
    }

    /* Disable migratory line feature to force PoC */
    __asm__ __volatile__ ("MRC p15, 4, %0, c15, c0, 0\n" : "=r"(PERIPHBASE));
    base = ioremap_nocache(PERIPHBASE, 4 * 1024);
    p = (unsigned int *) (base + 0x30);
    *p = (*p | 0x01);
    iounmap(base);


    return 0;
}

/* Entry of rmmod */
static void wbmod_exit(void)
{
    dev_t dev = MKDEV(wbmod_major, MINOR_BASE);

    /* Remove from kernel */
    cdev_del(&wbmod_cdev);

    /* Clear major number */
    unregister_chrdev_region(dev, MINOR_NUM);

    printk(KERN_ALERT "wbmod_exit is called\n");

}

module_init(wbmod_init);
module_exit(wbmod_exit);
