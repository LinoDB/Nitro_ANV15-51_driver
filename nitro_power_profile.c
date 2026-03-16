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
    // .gmInput = 0 // misc input - id 9 power profile: 0x2
    // .gmInput = 5 // misc input - id 23 overclocking: 0x1
    // .gmInput = 7 // misc input - id 3 upper profile shizzle: 0xff00
    // .gmInput = 10 // misc input - ???: 0x5300 = available profiles (mask: 0, 1, 4, 6)
    .gmInput = 11 // misc input - id 3 lower profile shizzle: 0x100
};

const struct wmi_method_input read_power_profile = {
    .in = { sizeof(struct profile_get_in), &check_profile_in },
    .instance = 0,
    // .method_id = 3, // '0xff000001000000' when in is 0, else '0x2'
    // .method_id = 5, // 0x227000000 for 0
    // .method_id = 9, // 0x2 for 0
    .method_id = 23
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
    .write = nitro_profile_write,
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
    u64 profile;
    union acpi_object* obj = run_wmi_command(nitro_profile_char_dev.wdev, &read_power_profile, sizeof(struct profile_get_out), "Read current power profile");
    if(obj) {
        profile = ((struct profile_get_out*)obj->buffer.pointer)->gmOutput;
        kfree(obj);
    }
    else {
        return -EFAULT;
    }
    if(profile & 0xFF) {
        // status check failed
        return -EAGAIN;
    }
    profile = profile >> 8;
    char* ret;
    size_t actual_count = 0;
    switch(profile) {
        case 0:
            ret = "quiet\n";
            actual_count = 6 * sizeof(char);
            break;
        case 1:
            ret = "balanced\n";
            actual_count = 9 * sizeof(char);
            break;
        case 4:
            ret = "performance\n";
            actual_count = 12 * sizeof(char);
            break;
        case 6:
            ret = "eco\n";
            actual_count = 4 * sizeof(char);
            break;
        default:
            printk(KERN_ERR "Found unknown nitro power profile number '0x%llx'", profile);
            ret = "unknown\n";
            actual_count = 8 * sizeof(char);
    }
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
    printk(KERN_INFO "Method id '%d' gives overclocking OCStructure '0x%x'", read_overclocking.method_id, overclocking.OCStructure[0]);


    if(copy_to_user(buf, "All good\n", 9)) {
        return -EFAULT;
    }
    *ppos += 9;
    return 9;
}

ssize_t nitro_profile_write(
    struct file* file,
    const char __user* buf,
    size_t count,
    loff_t* ppos
) {
    char activate[11];
    if(copy_from_user(activate, buf, 11)) {
        return -EFAULT;
    }
    struct profile_write_in write_in = {
        .gmInput = 1
    };

    if(!init_str_equal(activate, "quiet", 5)) {
        write_in.gmInput = 0;
    }
    else if(!init_str_equal(activate, "balanced", 8)) {
        write_in.gmInput = 1;
    }
    else if(!init_str_equal(activate, "performance", 11)) {
        write_in.gmInput = 4;
    }
    else if(!init_str_equal(activate, "eco", 3)) {
        write_in.gmInput = 6;
    }
    else {
        printk(KERN_WARNING "Undefined input for setting Nitro power profile: %s\n", activate);
        return -EINVAL;
    }
    write_in.gmInput = write_in.gmInput << 8;
    write_in.gmInput |= 0x0B;

    struct wmi_method_input set_platform_profile = {
        .in = { sizeof(struct profile_write_in), &write_in },
        .instance = 0,
        .method_id = 22
    };
    if(down_interruptible(&nitro_profile_lock)) return -ERESTARTSYS;
    union acpi_object* obj = run_wmi_command(nitro_profile_char_dev.wdev, &set_platform_profile, sizeof(struct profile_write_out), "Set gaming profile");
    up(&nitro_profile_lock);
    // ignore output
    if(obj) {
        printk(KERN_WARNING "Setting power profile gave back: %x\n", (*((struct profile_write_out*)obj->buffer.pointer)).gmOutput);
        kfree(obj);
        return count;
    }
    return -EFAULT;
}
