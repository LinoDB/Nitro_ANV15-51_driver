#include "misc.h"

#include <linux/acpi.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/wmi.h>


/************************************
************ WMI drivers ************
************************************/

const struct wmi_device_id battery_dev_id = {
    .guid_string = BATTERY_WMI_DEVICE_GUID
};

struct wmi_driver battery_driver = {
    .driver = {
        .name = "Battery WMI driver",
        .owner = THIS_MODULE,        
        .probe_type = PROBE_PREFER_ASYNCHRONOUS
    },
    .id_table = &battery_dev_id,
    .probe = &battery_wmi_device_probe,
    .remove = &battery_wmi_device_remove,     // needed?
    // .shutdown = &battery_wmi_device_remove    // not yet in LTS Kernel - needed?
};


const struct wmi_device_id gaming_dev_id = {
    .guid_string = GAMING_WMI_DEVICE_GUID
};

struct wmi_driver gaming_driver = {
    .driver = {
        .name = "Gaming WMI driver",
        .owner = THIS_MODULE,        
        .probe_type = PROBE_PREFER_ASYNCHRONOUS
    },
    .id_table = &gaming_dev_id,
    .probe = &gaming_wmi_device_probe,
    .remove = &gaming_wmi_device_remove,     // needed?
    // .shutdown = &gaming_wmi_device_remove    // not yet in LTS Kernel - needed?
};


/************************************
********* Device management *********
************************************/

int all_dev_uevent(const struct device *dev, struct kobj_uevent_env *env) {
    // give the device created read+write permissions 666
    add_uevent_var(env, "DEVMODE=%#o", 0666);
    return 0;
}


/************************************
****** File operation methods *******
************************************/

int nitro_open(struct inode* inode, struct file* file) {
    // obtain 'nitro_char_dev' from the cdev structure
    file->private_data = container_of(inode->i_cdev, struct nitro_char_dev, cdev);
    return 0;
}

int nitro_release(struct inode* node, struct file* file) {
    return 0;
}


/************************************
************ WMI methods ************
************************************/

union acpi_object* run_wmi_command(struct wmi_device* wdev, const struct wmi_method_input* input, size_t length, const char* call_name) {
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
********** Helper methods ***********
************************************/

bool init_str_equal(char* a, char* b, int len) {
    for(int i = 0; i < len; i++) {
        if(a[i] != b[i]) return false;
    }
    return true;
}
