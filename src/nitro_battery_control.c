#include <asm-generic/errno-base.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/kdev_t.h>
#include <linux/cdev.h>
#include <linux/miscdevice.h>
#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <asm/uaccess.h>
#include <linux/errno.h>


/************************************
* File operation method definitions *
************************************/

static ssize_t nitro_battery_read(
    struct file* file,
    char __user* buf,
    size_t count,
    loff_t* ppos
);

static ssize_t nitro_battery_write(
    struct file* file,
    const char __user* buf,
    size_t count,
    loff_t* ppos
);

static int nitro_battery_open(struct inode* inode, struct file* file);

static int nitro_battery_release(struct inode* node, struct file* file) {
    return 0;
}


/************************************
*** Device and utility structures ***
************************************/

static struct file_operations fops = {
    .owner = THIS_MODULE,
    .read = nitro_battery_read,
    .write = nitro_battery_write,
    .open = nitro_battery_open,
    .release = nitro_battery_release
};

struct nitro_battery {
    struct cdev cdev;
    struct class *dev_cl;
    dev_t devno;
    int major;
    int minor;
    bool initialized;
    bool active;
    const char* name;
};

static struct nitro_battery _device = {
    .initialized = false,
    .active = false,
    .name = "nitro_battery"
};


/************************************
****** File operation methods *******
************************************/

static int nitro_battery_open(struct inode* inode, struct file* file) {
    // obtain 'nitro_battery' from the cdev structure
    file->private_data = container_of(inode->i_cdev, struct nitro_battery, cdev);
    return 0;
}

static ssize_t nitro_battery_read(
    struct file* file,
    char __user* buf,
    size_t count,
    loff_t* ppos
) {
    if(*ppos > 0) return 0;
    struct nitro_battery* dev = file->private_data;
    char* ret;
    size_t actual_count = 0;
    if(dev->active) {
        ret = "activated\n";
        actual_count = 10 * sizeof(char);
    }
    else {
        ret = "deactivated\n";
        actual_count = 12 * sizeof(char);
    }
    if(copy_to_user(buf, ret, actual_count)) {
        return -EFAULT;
    }
    *ppos += actual_count;
    return actual_count;
}

static ssize_t nitro_battery_write(
    struct file* file,
    const char __user* buf,
    size_t count,
    loff_t* ppos
) {
    struct nitro_battery* dev = file->private_data;
    char activate[1];
    if(copy_from_user(activate, buf, 1)) {
        return -EFAULT;
    }
    char compstr[] = "01";
    if(*activate == compstr[0]) {
        dev->active = false;
    }
    else if(*activate == compstr[1]) {
        dev->active = true;
    }
    else {
        printk(KERN_WARNING "Undefined input for Nitro Battery Control Driver: %s\n", activate);
    }
    return count;
}


/************************************
**** Initialization and cleanup *****
************************************/

static int all_dev_uevent(const struct device *dev, struct kobj_uevent_env *env) {
    // give the device created read+write permissions 666
    add_uevent_var(env, "DEVMODE=%#o", 0666);
    return 0;
}

static void unregister_device(struct nitro_battery* dev, struct class* cl) {
    device_destroy(cl, MKDEV(_device.major, _device.minor));
    class_destroy(cl);
    dev->initialized = false;
}

static int __init nitro_battery_init(void) {
    int err = alloc_chrdev_region(&_device.devno, 0, 1, _device.name);
    _device.major = MAJOR(_device.devno);
    _device.minor = MINOR(_device.devno);
    if(err < 0) {
        printk(KERN_ERR "Nitro Battery Control Driver init error: couldn't get major %d\n", _device.major);
        return err;
    }

    _device.dev_cl = class_create(_device.name);
    if(_device.dev_cl == NULL) {
        printk(KERN_ERR "Nitro Battery Control Driver init error: device class couldn't be created\n");
        err = -1;
        goto unreg_region;
    }
    _device.dev_cl->dev_uevent = all_dev_uevent;

    if(device_create(_device.dev_cl, NULL, MKDEV(_device.major, _device.minor), NULL, _device.name) == NULL) {
        printk(KERN_ERR "Nitro Battery Control Driver init error: device couldn't be created\n");
        err = -1;
        goto unreg_dev;
    }
    _device.initialized = true;

    // create c_dev device
    cdev_init(&_device.cdev, &fops);
    _device.cdev.owner = THIS_MODULE;
    _device.cdev.ops = &fops; // necessary?

    // register c_dev device
    err = cdev_add(&_device.cdev, _device.devno, 1);
    if(err) {
        printk(KERN_ERR "Nitro Battery Control Driver init error: couldn't add c_dev: %d\n", err);
        goto unreg_dev;
    }

    printk(KERN_INFO "Initialized Nitro Battery Control driver with major number: %d!\n", _device.major);
    return 0;

    unreg_dev: unregister_device(&_device, _device.dev_cl);
    unreg_region: unregister_chrdev_region(_device.devno, 1);
    return err;
}

static void __exit nitro_battery_exit(void) {
    if(_device.initialized) {
        cdev_del(&_device.cdev);
        unregister_device(&_device, _device.dev_cl);
        unregister_chrdev_region(_device.devno, 1);
        printk(KERN_INFO "Removed Nitro Battery Control driver!\n");
    }
    else {
        printk(KERN_INFO "Nitro Battery Control driver wasn't initialized properly!\n");
    }
}

module_init(nitro_battery_init);
module_exit(nitro_battery_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Lino Del Bianco");
MODULE_DESCRIPTION("Nitro ANV15-51 battery control");
