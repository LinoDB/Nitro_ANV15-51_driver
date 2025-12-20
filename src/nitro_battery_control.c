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
#include <linux/acpi.h>
#include <linux/wmi.h>

#define BATTERY_CONTROL_GUID "79772EC5-04B1-4bfd-843C-61E7F77B6CC9"


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
    const char* name;
};

static struct nitro_battery _device = {
    .initialized = false,
    .name = "nitro_battery_charge_limit"
};

static const struct wmi_device_id batt_dev_id = {
    .guid_string = BATTERY_CONTROL_GUID
};

static struct wmi_device* batt_wdev;

static int batt_probe(struct wmi_device *wdev, const void *context) {
    if(!wmi_has_guid(BATTERY_CONTROL_GUID)) {
        printk(KERN_ERR "Nitro Battery Control Driver init error: WMI device GUID couldn't be found");
        return -ENODEV;
    }
    batt_wdev = wdev;
    return 0;
}

static void batt_remove(struct wmi_device *wdev) {
    if(batt_wdev) batt_wdev = NULL;
}

static struct wmi_driver batt_driver = {
    .driver = {
        .owner = THIS_MODULE,
        .name = "Battery health WMI driver",
        .probe_type = PROBE_PREFER_ASYNCHRONOUS
    },
    .id_table = &batt_dev_id,
    .probe = &batt_probe,
    .remove = &batt_remove,     // needed?
    .shutdown = &batt_remove    // needed?
};

struct __attribute__((packed)) battery_get_charge_limit_in {
    u8 uBatteryNo;
    u8 uFunctionQuery;
    u8 uReserved[2];
};

static struct battery_get_charge_limit_in check_charge_limit_in = {
    .uBatteryNo = 1,
    .uFunctionQuery = 1,
    .uReserved = {0, 0}
};

struct __attribute__((packed)) battery_get_charge_limit_out {
    u8 uFunctionList;
    u8 uReturn[2];
    u8 uFunctionStatus[5];
};

struct __attribute__((packed)) battery_set_charge_limit_in {
    u8 uBatteryNo;
    u8 uFunctionMask;
    u8 uFunctionStatus;
    u8 uReservedIn[5];
};

static struct battery_set_charge_limit_in set_charge_limit_in = {
    .uBatteryNo = 1,
    .uFunctionMask = 1,
    .uFunctionStatus = 0,
    .uReservedIn = {0, 0, 0, 0, 0}
};

struct __attribute__((packed)) battery_set_charge_limit_out {
    u16 uReturn;
    u16 uReservedOut;
};

struct wmi_method_input {
    const struct acpi_buffer in;
    u8 instance;
    u32 method_id;
};

static const struct wmi_method_input read_battery_charge_limited = {
    .in = { sizeof(struct battery_get_charge_limit_in), &check_charge_limit_in },
    .instance = 0,
    .method_id = 20
};

static const struct wmi_method_input write_battery_charge_limited = {
    .in = { sizeof(struct battery_set_charge_limit_in), &set_charge_limit_in },
    .instance = 0,
    .method_id = 21
};

u8* set_battery_limit = &set_charge_limit_in.uFunctionStatus;


/************************************
************ WMI methods ************
************************************/


static union acpi_object* run_wmi_command(struct wmi_device* wdev, const struct wmi_method_input* input, size_t length, const char* call_name) {
    if(!wdev) return NULL;
    struct acpi_buffer out = { ACPI_ALLOCATE_BUFFER, NULL };
    acpi_status status = wmidev_evaluate_method(
        wdev,
        input->instance,
        input->method_id,
        &input->in,
        &out
    );
    if(ACPI_FAILURE(status)) {
        printk(KERN_ERR "Nitro Control Driver: Couldn't evaluate wmidev method %d (%s)\n", input->method_id, call_name);
        return NULL;
    }
    // thanks to 0x7375646F: https://github.com/0x7375646F/Linuwu-Sense/blob/df84ac7a020efebd4cd1097e73940d93eb959093/src/linuwu_sense.c#L3001
    union acpi_object* obj = out.pointer;
    if (!obj || obj->type != ACPI_TYPE_BUFFER || obj->buffer.length != length) {
        printk(
            KERN_ERR "Nitro Control Driver: '%s' returned wrong buffer: type %d, length %d\n",
            call_name,
            obj->type,
            obj->buffer.length
        );
        kfree(obj);
        return NULL;
    }
    return obj;
}


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
    u8 enabled = -1;
    union acpi_object* obj = run_wmi_command(batt_wdev, &read_battery_charge_limited, sizeof(struct battery_get_charge_limit_out), "Read battery charge limit");
    if(obj) {
        enabled = ((struct battery_get_charge_limit_out*)obj->buffer.pointer)->uFunctionStatus[0];
        kfree(obj);
    }
    char* ret;
    size_t actual_count = 0;
    switch(enabled) {
        case 1:
            ret = "80%\n";
            actual_count = 4 * sizeof(char);
            break;
        case 0:
            ret = "100%\n";
            actual_count = 5 * sizeof(char);
            break;
        default:
            ret = "unknown\n";
            actual_count = 8 * sizeof(char);
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
    // struct nitro_battery* dev = file->private_data;
    char activate[1];
    if(copy_from_user(activate, buf, 1)) {
        return -EFAULT;
    }
    switch(*activate) {
        case '0':
            *set_battery_limit = 0;
            break;
        case '1':
            *set_battery_limit = 1;
            break;
        default:
            printk(KERN_WARNING "Undefined input for setting Nitro battery control mode: %s\n", activate);
            return -EINVAL;
    }
    union acpi_object* obj = run_wmi_command(batt_wdev, &write_battery_charge_limited, sizeof(struct battery_set_charge_limit_out), "Set battery charge limit");
    // ignore output
    if(obj) {
        kfree(obj);
        return count;
    }
    return -EFAULT;
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
    _device.cdev.ops = &fops; // needed?

    // register c_dev device
    err = cdev_add(&_device.cdev, _device.devno, 1);
    if(err) {
        printk(KERN_ERR "Nitro Battery Control Driver init error: couldn't add c_dev: %d\n", err);
        goto unreg_dev;
    }

    if(wmi_driver_register(&batt_driver)) {
        printk(KERN_ERR "Nitro Battery Control Driver init error: Couldn't register battery device driver");
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
    if(batt_wdev) {
        wmi_driver_unregister(&batt_driver);
    }
}

module_init(nitro_battery_init);
module_exit(nitro_battery_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Lino Del Bianco");
MODULE_DESCRIPTION("Nitro ANV15-51 battery control");
