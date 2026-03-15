#include "nitro_power_profile.h"

#include <linux/acpi.h>
#include <linux/errno.h>
#include <linux/fs.h>
#include <linux/kdev_t.h>
#include <linux/kernel.h>
#include <linux/semaphore.h>
#include <linux/wmi.h>


DEFINE_SEMAPHORE(nitro_profile_lock, 1);


/************************************
******* WMI evaluation in/out *******
************************************/

struct profile_get_in check_profile_in = {
    .gmInput = 0 // misc input - id 9 power profile: 0x2
    // .gmInput = 5 // misc input - id 23 overclocking: 0x1
    // .gmInput = 7 // misc input - id 3 upper profile shizzle: 0xff00
    // .gmInput = 10 // misc input - ???: 0x5300
    // .gmInput = 11 // misc input - id 3 lower profile shizzle: 0x100
};

const struct wmi_method_input read_power_profile = {
    .in = { sizeof(struct profile_get_in), &check_profile_in },
    .instance = 0,
    .method_id = 3, // '0xff000001000000' when in is 0, else '0x2'
    // .method_id = 5,
    // .method_id = 9,
    // .method_id = 23
};

struct overclocking_get_in check_overclocking_in = {
    .Reserved = {0, 0, 0, 0}
};

const struct wmi_method_input read_overclocking = {
    .in = { sizeof(struct overclocking_get_in), &check_overclocking_in },
    .instance = 0,
    .method_id = 25,
};


/************************************
********* Device management *********
************************************/

struct file_operations prof_fops = {
    .owner = THIS_MODULE,
    .read = nitro_profile_read,
    // .read = nitro_overclock_read,
    // .write = nitro_battery_write,
    .open = nitro_open,
    .release = nitro_release
};

const struct wmi_device_id prof_dev_id = {
    .guid_string = POWER_PROFILE_GUID
};

struct wmi_driver prof_driver = {
    .driver = {
        .name = "Power profile WMI driver",
        .owner = THIS_MODULE,        
        .probe_type = PROBE_PREFER_ASYNCHRONOUS
    },
    .id_table = &prof_dev_id,
    .probe = &prof_probe,
    .remove = &prof_remove,     // needed?
    // .shutdown = &prof_remove    // not yet in LTS Kernel - needed?
};

struct nitro_char_dev nitro_profile_char_dev = {
    .fops = &prof_fops,
    .driver = &prof_driver,
    .name = "Power Profile Controller",
    .file_name = "nitro_power_profile",
    .initialized = false,
};


/************************************
********** Driver methods ***********
************************************/

int prof_probe(struct wmi_device *wdev, const void *context) {
    if(!wmi_has_guid(POWER_PROFILE_GUID)) {
        printk(KERN_ERR "Nitro Power Profile Control Driver init error: WMI device GUID couldn't be found");
        return -ENODEV;
    }
    nitro_profile_char_dev.wdev = wdev;
    return 0;
}

void prof_remove(struct wmi_device *wdev) {
    if(nitro_profile_char_dev.wdev) nitro_profile_char_dev.wdev = NULL;
}


/************************************
****** File operation methods *******
************************************/

ssize_t nitro_profile_read(
    struct file* file,
    char __user* buf,
    size_t count,
    loff_t* ppos
) {
    if(*ppos > 0) return 0;
    u64 profile = 123456;
    union acpi_object* obj = run_wmi_command(nitro_profile_char_dev.wdev, &read_power_profile, sizeof(struct profile_get_out), "Read current power profile");
    if(obj) {
        profile = ((struct profile_get_out*)obj->buffer.pointer)->gmOutput;
        kfree(obj);
    }
    else {
        return -EFAULT;
    }
    printk(KERN_INFO "Method id '%d' gives power profile number '0x%llx'", read_power_profile.method_id, profile);
    char ret[9];
    size_t actual_count = 9 * sizeof(char);
    for(int i = 0; i < 8; i++) {
        ret[i] = (u8) (profile & 7);
        profile = profile >> 3;
    }
    ret[8] = '\n';
    // printk(KERN_INFO "As string: %s", ret);
    // ret[9] = '\0';
    // char* ret;
    // size_t actual_count = 0;
    // switch(profile) {
    //     case 1:
    //         ret = "80%\n";
    //         actual_count = 4 * sizeof(char);
    //         break;
    //     case 0:
    //         ret = "100%\n";
    //         actual_count = 5 * sizeof(char);
    //         break;
    //     default:
    //         ret = "unknown\n";
    //         actual_count = 8 * sizeof(char);
    // }
    if(copy_to_user(buf, ret, actual_count)) {
        return -EFAULT;
    }
    *ppos += actual_count;
    return actual_count;
}

ssize_t nitro_overclock_read(
    struct file* file,
    char __user* buf,
    size_t count,
    loff_t* ppos
) {
    if(*ppos > 0) return 0;
    struct overclocking_get_out overclocking;
    union acpi_object* obj = run_wmi_command(nitro_profile_char_dev.wdev, &read_overclocking, sizeof(struct overclocking_get_out), "Read overclocking profile");
    if(obj) {
        overclocking = *((struct overclocking_get_out*)obj->buffer.pointer);
        kfree(obj);
    }
    else {
        return -EFAULT;
    }
    printk(KERN_INFO "Method id '%d' gives overclocking ReturnCode '0x%x'", read_overclocking.method_id, overclocking.ReturnCode);
    printk(KERN_INFO "Method id '%d' gives overclocking ReturnOCProfile '0x%x'", read_overclocking.method_id, overclocking.ReturnOCProfile);


    if(copy_to_user(buf, "All good\n", 9)) {
        return -EFAULT;
    }
    *ppos += 9;
    return 9;
}

// ssize_t nitro_battery_write(
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
//     .uBatteryNo = 1,
//     .uFunctionMask = 1,
//     .uFunctionStatus = 0,
//     .uReservedIn = {0, 0, 0, 0, 0}
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
//         .method_id = 21
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
