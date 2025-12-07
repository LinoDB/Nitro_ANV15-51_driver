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


// FILE OPERATIONS METHODS

static ssize_t char_dev_test_read(
    struct file* file,
    char __user* buf,
    size_t count,
    loff_t* ppos
);

static ssize_t char_dev_test_write(
    struct file* file,
    const char __user* buf,
    size_t count,
    loff_t* ppos
);

static int char_dev_test_open(struct inode* inode, struct file* file); 

static int char_dev_test_release(struct inode* node, struct file* file) {
    return 0;
}

// DEVICE AND UTILITY STRUCTS

static struct file_operations fops = {
    .owner = THIS_MODULE,
    .read = char_dev_test_read,
    .write = char_dev_test_write,
    .open = char_dev_test_open,
    .release = char_dev_test_release
};

struct char_dev_test {
    struct cdev cdev;
    struct class *dev_cl;
    dev_t devno;
    uint devcount;
    int major;
    int minor;
    int initialized;
    bool active;
    const char* name;
};

static int char_dev_test_open(struct inode* inode, struct file* file) {
    // obtain the 'char_dev_test' from the cdev structure
    file->private_data = container_of(inode->i_cdev, struct char_dev_test, cdev);
    return 0;
}

static ssize_t char_dev_test_read(
    struct file* file,
    char __user* buf,
    size_t count,
    loff_t* ppos
) {
    if(*ppos > 0) return 0;
    struct char_dev_test* dev = file->private_data;
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

static ssize_t char_dev_test_write(
    struct file* file,
    const char __user* buf,
    size_t count,
    loff_t* ppos
) {
    struct char_dev_test* dev = file->private_data;
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
        printk(KERN_WARNING "Undefined input for CHAR DEV TEST: %s\n", activate);
    }
    return count;
}

static struct char_dev_test test_dev = {
    .initialized = 0,
    .devcount = 2,
    .active = false,
    .name = "char_dev_test"
};


// INITIALIZATION AND CLEANUP

static int all_dev_uevent(const struct device *dev, struct kobj_uevent_env *env) {
    add_uevent_var(env, "DEVMODE=%#o", 0666);
    return 0;
}

static void unregister_devices(struct char_dev_test* dev, struct class* cl) {
    for(int i = dev->initialized; i > 0; i--) {
        device_destroy(cl, MKDEV(test_dev.major, test_dev.minor + i - 1));
    }
    class_destroy(cl);
    dev->initialized = 0;
}

static int __init char_dev_test_init(void) {
    int err = alloc_chrdev_region(&test_dev.devno, 0, test_dev.devcount, test_dev.name);
    test_dev.major = MAJOR(test_dev.devno);
    test_dev.minor = MINOR(test_dev.devno);
    if(err < 0) {
        printk(KERN_ERR "CHAR DEV TEST init error: couldn't get major %d\n", test_dev.major);
        return err;
    }

    test_dev.dev_cl = class_create(test_dev.name);
    if(test_dev.dev_cl == NULL) {
        printk(KERN_ERR "CHAR DEV TEST init error: device class couldn't be created\n");
        err = -1;
        goto unreg_region;
    }
    test_dev.dev_cl->dev_uevent = all_dev_uevent;

    for(int i = 0; i < 2; i++) {
        if(device_create(test_dev.dev_cl, NULL, MKDEV(test_dev.major, test_dev.minor + i), NULL, "%s_%d", test_dev.name, i + 1) == NULL) {
            printk(KERN_ERR "CHAR DEV TEST init error: device number %d couldn't be created\n", i + 1);
            err = -1;
            goto unreg_dev;
        }
        test_dev.initialized++;
    }

    // create c_dev device
    cdev_init(&test_dev.cdev, &fops);
    test_dev.cdev.owner = THIS_MODULE;
    test_dev.cdev.ops = &fops; // necessary?

    // register c_dev device
    err = cdev_add(&test_dev.cdev, test_dev.devno, test_dev.devcount);
    if(err) {
        printk(KERN_ERR "CHAR DEV TEST init error: couldn't add c_dev: %d\n", err);
        goto unreg_dev;
    }

    printk(KERN_INFO "Initialized CHAR DEV TEST device with major number: %d!\n", test_dev.major);
    return 0;

    unreg_dev: unregister_devices(&test_dev, test_dev.dev_cl);
    unreg_region: unregister_chrdev_region(test_dev.devno, test_dev.devcount);
    return err;
}

static void __exit char_dev_test_exit(void) {
    if(test_dev.initialized) {
        cdev_del(&test_dev.cdev);
        unregister_devices(&test_dev, test_dev.dev_cl);
        unregister_chrdev_region(test_dev.devno, test_dev.devcount);
        printk(KERN_INFO "Removed CHAR DEV TEST device!\n");
    }
    else {
        printk(KERN_INFO "CHAR DEV TEST wasn't initialized properly!\n");
    }
}

module_init(char_dev_test_init);
module_exit(char_dev_test_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Me Lol");
MODULE_DESCRIPTION("character device module");
