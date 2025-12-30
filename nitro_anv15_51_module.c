#include "misc.h"
#include "nitro_anv15_51_module.h"

#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/err.h>
#include <linux/init.h>
#include <linux/kdev_t.h>
#include <linux/module.h>
#include <linux/wmi.h>


/************************************
****** Major device structure *******
************************************/

extern struct nitro_char_dev nitro_battery_char_dev;

struct nitro_char_dev* character_devices[] = {
    &nitro_battery_char_dev,
};

struct nitro_anv15_51 _device = {
    .char_device_count = sizeof(character_devices)/sizeof(struct nitro_char_dev*),
    .initialized = false,
    .name = "Nitro ANV15-51 driver",
    .char_devs = character_devices,
};


/************************************
**** Initialization and cleanup *****
************************************/

static int __init nitro__av15_51_init(void) {
    int err = alloc_chrdev_region(&_device.devno, 0, _device.char_device_count, _device.name);
    _device.major = MAJOR(_device.devno);
    if(err < 0) {
        printk(KERN_ERR "Nitro ANV15-51 driver init error: couldn't get major number %d\n", _device.major);
        return err;
    }
    
    for(int i = 0; i < _device.char_device_count; i++) {
        struct nitro_char_dev* char_dev = _device.char_devs[i];
        char_dev->minor = i;

        if(!wmi_has_guid(char_dev->driver->id_table->guid_string)) {
            printk(KERN_ERR "Nitro ANV15-51 driver %s init error: WMI device GUID '%s' doesn't exist", char_dev->name, char_dev->driver->id_table->guid_string);
            continue;
        }
        if(wmi_driver_register(char_dev->driver)) {
            printk(KERN_ERR "Nitro ANV15-51 driver %s init error: Couldn't register WMI driver", char_dev->name);
            continue;
        }

        char_dev->dev_cl = class_create(char_dev->file_name);
        if(IS_ERR(char_dev->dev_cl)) {
            printk(KERN_ERR "Nitro ANV15-51 driver %s init error: device class couldn't be created\n", char_dev->name);
            goto wmi_unreg;
        }
        char_dev->dev_cl->dev_uevent = all_dev_uevent;

        if(IS_ERR(device_create(char_dev->dev_cl, NULL, MKDEV(_device.major, char_dev->minor), NULL, char_dev->file_name))) {
            printk(KERN_ERR "Nitro ANV15-51 driver %s init error: device couldn't be created\n", char_dev->name);
            goto destroy_class;
        }

        // create c_dev device
        cdev_init(&char_dev->cdev, char_dev->fops);
        char_dev->cdev.owner = THIS_MODULE;
        char_dev->cdev.ops = char_dev->fops; // needed?

        // register c_dev device
        err = cdev_add(&char_dev->cdev, MKDEV(_device.major, char_dev->minor), 1);
        if(err) {
            printk(KERN_ERR "Nitro ANV15-51 driver %s init error: couldn't add c_dev: %d\n", char_dev->name, err);
            goto destroy_dev;
        }
        char_dev->initialized = true;
        _device.initialized = true;
        continue;
        destroy_dev: device_destroy(char_dev->dev_cl, MKDEV(_device.major, char_dev->minor));
        destroy_class: class_destroy(char_dev->dev_cl);
        wmi_unreg: wmi_driver_unregister(char_dev->driver);
    }

    if(_device.initialized) {
        printk(KERN_INFO "Initialized Nitro ANV15-51 driver with major number: %d\n", _device.major);
        return 0;
    }
    printk(KERN_ERR "Nitro ANV15-51 driver devices couldn't be initialized");
    unregister_chrdev_region(_device.devno, _device.char_device_count);
    return -1;
}

static void __exit nitro__av15_51_exit(void) {
    if(_device.initialized) {
        for(int i = 0; i < _device.char_device_count; i++) {
            struct nitro_char_dev* char_dev = _device.char_devs[i];
            if(!char_dev->initialized) {
                continue;
            }
            cdev_del(&char_dev->cdev);
            device_destroy(char_dev->dev_cl, MKDEV(_device.major, char_dev->minor));
            class_destroy(char_dev->dev_cl);
            wmi_driver_unregister(char_dev->driver);
            char_dev->initialized = false;
        }
        unregister_chrdev_region(_device.devno, _device.char_device_count);
        _device.initialized = false;
        printk(KERN_INFO "Removed Nitro ANV15-51 driver\n");
    }
    else {
        printk(KERN_INFO "Nitro ANV15-51 driver wasn't initialized properly\n");
    }
}

module_init(nitro__av15_51_init);
module_exit(nitro__av15_51_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Lino Del Bianco");
MODULE_DESCRIPTION("Nitro ANV15-51 driver");
