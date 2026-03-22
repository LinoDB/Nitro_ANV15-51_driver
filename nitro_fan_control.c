#include "nitro_fan_control.h"

#include <linux/acpi.h>
#include <linux/errno.h>
#include <linux/fs.h>
#include <linux/kdev_t.h>
#include <linux/kernel.h>
#include <linux/semaphore.h>
#include <linux/wmi.h>


DEFINE_SEMAPHORE(nitro_fan_lock, 1);


/************************************
******* WMI evaluation in/out *******
************************************/

struct get_fan_behaviour_in check_fan_behaviour_in = {
    .gmInput = 0
};

const struct wmi_method_input read_fan_behaviour = {
    .in = { sizeof(struct get_fan_behaviour_in), &check_fan_behaviour_in },
    .instance = 0,
    .method_id = FAN_GET_BEHAVIOUR_METHOD_ID
};


struct get_fan_speed_in check_fan_speed_in = {
    .gmInput = 0
};

const struct wmi_method_input read_fan_speed = {
    .in = { sizeof(struct get_fan_speed_in), &check_fan_speed_in },
    .instance = 0,
    .method_id = FAN_GET_SPEED_METHOD_ID
};


/************************************
********* Device management *********
************************************/

struct file_operations fan_fops = {
    .owner = THIS_MODULE,
    .read = nitro_fan_read,
    // .write = nitro_fan_write,
    .open = nitro_open,
    .release = nitro_release
};

const struct wmi_device_id fan_dev_id = {
    .guid_string = FAN_CONTROL_GUID
};

struct wmi_driver fan_driver = {
    .driver = {
        .name = "Fan speed WMI driver",
        .owner = THIS_MODULE,        
        .probe_type = PROBE_PREFER_ASYNCHRONOUS
    },
    .id_table = &fan_dev_id,
    .probe = &fan_probe,
    .remove = &fan_remove,     // needed?
    // .shutdown = &fan_remove    // not yet in LTS Kernel - needed?
};

struct nitro_char_dev nitro_fan_char_dev = {
    .fops = &fan_fops,
    .driver = &fan_driver,
    .name = "Fan Controller",
    .file_name = "nitro_fan_control",
    .initialized = false,
};


/************************************
********** Driver methods ***********
************************************/

int fan_probe(struct wmi_device *wdev, const void *context) {
    if(!wmi_has_guid(FAN_CONTROL_GUID)) {
        printk(KERN_ERR "Nitro Fan Control Driver init error: WMI device GUID couldn't be found");
        return -ENODEV;
    }
    nitro_fan_char_dev.wdev = wdev;
    return 0;
}

void fan_remove(struct wmi_device *wdev) {
    if(nitro_fan_char_dev.wdev) nitro_fan_char_dev.wdev = NULL;
}


/************************************
****** File operation methods *******
************************************/

ssize_t nitro_fan_read(
    struct file* file,
    char __user* buf,
    size_t count,
    loff_t* ppos
) {
    if(*ppos > 0) return 0;
    u64 behaviour;
    union acpi_object* obj = run_wmi_command(nitro_fan_char_dev.wdev, &read_fan_behaviour, sizeof(struct get_fan_behaviour_out), "Read fan behaviour");
    if(obj) {
        behaviour = ((struct get_fan_behaviour_out*)obj->buffer.pointer)->gmOutput;
        kfree(obj);
    }
    else {
        return -EFAULT;
    }
    printk(KERN_INFO "Nitro fan behaviour: 0x%llx", behaviour);

    u64 speed;
    union acpi_object* obj2 = run_wmi_command(nitro_fan_char_dev.wdev, &read_fan_speed, sizeof(struct get_fan_speed_out), "Read fan speed");
    if(obj2) {
        speed = ((struct get_fan_speed_out*)obj2->buffer.pointer)->gmOutput;
        kfree(obj2);
    }
    else {
        return -EFAULT;
    }
    printk(KERN_INFO "Nitro fan speed: 0x%llx", speed);
    printk(KERN_INFO "FLUSH");

    char* ret = "OK\n";
    size_t actual_count = 3;
    // char* ret;
    // size_t actual_count = 0;
    // switch(enabled) {
    //     case 1:
    //         ret = "80%\n";
    //         actual_count = 4 * sizeof(char);
    //         break;
    //     case 0:
    //         ret = "100%\n";
    //         actual_count = 5 * sizeof(char);
    //         break;
    //     default:
    //         printk(KERN_ERR "Found unknown nitro battery charge limit number '%u'", enabled);
    //         ret = "unknown\n";
    //         actual_count = 8 * sizeof(char);
    // }
    if(copy_to_user(buf, ret, actual_count)) {
        return -EFAULT;
    }
    *ppos += actual_count;
    return actual_count;
}

// ssize_t nitro_fan_write(
//     struct file* file,
//     const char __user* buf,
//     size_t count,
//     loff_t* ppos
// ) {
//     // struct nitro_char_dev* dev = file->private_data;
//     char activate[1];
//     if(copy_from_user(activate, buf, 1)) {
//         return -EFAULT;
//     }
//     struct battery_set_charge_limit_in set_charge_limit_in = {
//         .uBatteryNo = 1,
//         .uFunctionMask = 1,
//         .uFunctionStatus = 0,
//         .uReservedIn = {0, 0, 0, 0, 0}
//     };

//     switch(*activate) {
//         case '0':
//             set_charge_limit_in.uFunctionStatus = 0;
//             break;
//         case '1':
//             set_charge_limit_in.uFunctionStatus = 1;
//             break;
//         default:
//             printk(KERN_WARNING "Undefined input for setting Nitro battery control mode: %s\n", activate);
//             return -EINVAL;
//     }
//     struct wmi_method_input write_battery_charge_limited = {
//         .in = { sizeof(struct battery_set_charge_limit_in), &set_charge_limit_in },
//         .instance = 0,
//         .method_id = BATTERY_SET_HEALTH_CONTROL_METHOD_ID
//     };
//     if(down_interruptible(&nitro_battery_lock)) return -ERESTARTSYS;
//     union acpi_object* obj = run_wmi_command(nitro_battery_char_dev.wdev, &write_battery_charge_limited, sizeof(struct battery_set_charge_limit_out), "Set battery charge limit");
//     up(&nitro_battery_lock);
//     // ignore output
//     if(obj) {
//         kfree(obj);
//         return count;
//     }
//     return -EFAULT;
// }
