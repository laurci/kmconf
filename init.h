#ifndef INIT_H
#define INIT_H

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/kdev_t.h>
#include <linux/version.h>
#include <linux/slab.h>

#include <linux/fs.h>
#include <linux/device.h>
#include <linux/cdev.h>

#define LOG_INFO(args...) printk(KERN_INFO "[config-device] " args)
#define LOG_ERR(args...) printk(KERN_ERR "[config-device] " args)

#endif