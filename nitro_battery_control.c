#include "nitro_battery_control.h"

#include <linux/acpi.h>
#include <linux/errno.h>
#include <linux/fs.h>
#include <linux/kdev_t.h>
#include <linux/kernel.h>
#include <linux/semaphore.h>
#include <linux/wmi.h>


DEFINE_SEMAPHORE(nitro_battery_lock, 1);


/************************************
******* WMI evaluation in/out *******
************************************/

struct battery_get_charge_limit_in check_charge_limit_in = {
    .uBatteryNo = 1,
    .uFunctionQuery = 1,
    .uReserved = {0, 0}
};

const struct wmi_method_input read_battery_charge_limited = {
    .in = { sizeof(struct battery_get_charge_limit_in), &check_charge_limit_in },
    .instance = 0,
    .method_id = 20
};


/************************************
********* Device management *********
************************************/

struct file_operations batt_fops = {
    .owner = THIS_MODULE,
    .read = nitro_battery_read,
    .write = nitro_battery_write,
    .open = nitro_open,
    .release = nitro_release
};

const struct wmi_device_id batt_dev_id = {
    .guid_string = BATTERY_CONTROL_GUID
};

struct wmi_driver batt_driver = {
    .driver = {
        .name = "Battery health WMI driver",
        .owner = THIS_MODULE,        
        .probe_type = PROBE_PREFER_ASYNCHRONOUS
    },
    .id_table = &batt_dev_id,
    .probe = &batt_probe,
    .remove = &batt_remove,     // needed?
    // .shutdown = &batt_remove    // not yet in LTS Kernel - needed?
};

struct nitro_char_dev nitro_battery_char_dev = {
    .fops = &batt_fops,
    .driver = &batt_driver,
    .name = "Battery Controller",
    .file_name = "nitro_battery_charge_limit",
    .initialized = false,
};


/************************************
********** Driver methods ***********
************************************/

int batt_probe(struct wmi_device *wdev, const void *context) {
    if(!wmi_has_guid(BATTERY_CONTROL_GUID)) {
        printk(KERN_ERR "Nitro Battery Control Driver init error: WMI device GUID couldn't be found");
        return -ENODEV;
    }
    nitro_battery_char_dev.wdev = wdev;
    return 0;
}

void batt_remove(struct wmi_device *wdev) {
    if(nitro_battery_char_dev.wdev) nitro_battery_char_dev.wdev = NULL;
}


/************************************
****** File operation methods *******
************************************/

ssize_t nitro_battery_read(
    struct file* file,
    char __user* buf,
    size_t count,
    loff_t* ppos
) {
    if(*ppos > 0) return 0;
    u8 enabled = 2;
    union acpi_object* obj = run_wmi_command(nitro_battery_char_dev.wdev, &read_battery_charge_limited, sizeof(struct battery_get_charge_limit_out), "Read battery charge limit");
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

ssize_t nitro_battery_write(
    struct file* file,
    const char __user* buf,
    size_t count,
    loff_t* ppos
) {
    // struct nitro_char_dev* dev = file->private_data;
    char activate[1];
    if(copy_from_user(activate, buf, 1)) {
        return -EFAULT;
    }
    struct battery_set_charge_limit_in set_charge_limit_in = {
    .uBatteryNo = 1,
    .uFunctionMask = 1,
    .uFunctionStatus = 0,
    .uReservedIn = {0, 0, 0, 0, 0}
    };

    switch(*activate) {
        case '0':
            set_charge_limit_in.uFunctionStatus = 0;
            break;
        case '1':
            set_charge_limit_in.uFunctionStatus = 1;
            break;
        default:
            printk(KERN_WARNING "Undefined input for setting Nitro battery control mode: %s\n", activate);
            return -EINVAL;
    }
    struct wmi_method_input write_battery_charge_limited = {
        .in = { sizeof(struct battery_set_charge_limit_in), &set_charge_limit_in },
        .instance = 0,
        .method_id = 21
    };
    if(down_interruptible(&nitro_battery_lock)) return -ERESTARTSYS;
    union acpi_object* obj = run_wmi_command(nitro_battery_char_dev.wdev, &write_battery_charge_limited, sizeof(struct battery_set_charge_limit_out), "Set battery charge limit");
    up(&nitro_battery_lock);
    // ignore output
    if(obj) {
        kfree(obj);
        return count;
    }
    return -EFAULT;
}
