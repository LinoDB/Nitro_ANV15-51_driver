#ifndef NITRO_ANV15_51_MISC_H
#define NITRO_ANV15_51_MISC_H

#include <linux/acpi.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/wmi.h>


/************************************
********** WMI structures ***********
************************************/

struct wmi_method_input {
    const struct acpi_buffer in;
    u8 instance;
    u32 method_id;
};

struct nitro_char_dev {
    struct class *dev_cl;
    struct cdev cdev;
    struct wmi_device* wdev;
    struct file_operations* fops;
    struct wmi_driver* driver;
    int minor;
    const char* name;
    const char* file_name;
    bool initialized;
};


/************************************
********* Device management *********
************************************/

void unregister_device(const unsigned int major, struct nitro_char_dev* char_dev);

int all_dev_uevent(const struct device *dev, struct kobj_uevent_env *env);


/************************************
************ WMI methods ************
************************************/

union acpi_object* run_wmi_command(struct wmi_device* wdev, const struct wmi_method_input* input, size_t length, const char* call_name);

#endif
