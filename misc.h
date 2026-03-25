#ifndef NITRO_ANV15_51_MISC_H
#define NITRO_ANV15_51_MISC_H

#include <linux/acpi.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/wmi.h>


#define BATTERY_WMI_DEVICE_GUID "79772EC5-04B1-4bfd-843C-61E7F77B6CC9"
#define GAMING_WMI_DEVICE_GUID "7A4DDFE7-5B5D-40B4-8595-4408E0CC7F56"


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
* File operation method definitions *
************************************/

int nitro_open(struct inode* inode, struct file* file);

int nitro_release(struct inode* node, struct file* file);


/************************************
********* Device management *********
************************************/

int all_dev_uevent(const struct device *dev, struct kobj_uevent_env *env);


/************************************
************ WMI methods ************
************************************/

union acpi_object* run_wmi_command(struct wmi_device* wdev, const struct wmi_method_input* input, size_t length, const char* call_name);

int battery_wmi_device_probe(struct wmi_device *wdev, const void *context);

void battery_wmi_device_remove(struct wmi_device *wdev);

int gaming_wmi_device_probe(struct wmi_device *wdev, const void *context);

void gaming_wmi_device_remove(struct wmi_device *wdev);


/************************************
********** Helper methods ***********
************************************/

bool init_str_equal(char* a, char* b, int len);

#endif
