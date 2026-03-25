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
    .gmInput = 11
};

const struct wmi_method_input read_power_profile = {
    .in = { sizeof(struct profile_get_in), &check_profile_in },
    .instance = 0,
    .method_id = POWER_PROFILE_GET_MISCELLANEOUS_SETTING_METHOD_ID
};


/************************************
********* Device management *********
************************************/

struct file_operations prof_fops = {
    .owner = THIS_MODULE,
    .read = nitro_profile_read,
    .write = nitro_profile_write,
    .open = nitro_open,
    .release = nitro_release
};

extern struct wmi_driver gaming_driver;

struct nitro_char_dev nitro_profile_char_dev = {
    .fops = &prof_fops,
    .driver = &gaming_driver,
    .name = "Power Profile Controller",
    .file_name = "nitro_anv15_51!power_profile",
    .initialized = false,
};


/************************************
****** File operation methods *******
************************************/

ssize_t nitro_profile_read(
    struct file* file,
    char __user* buf,
    size_t count,
    loff_t* ppos
) {
    // thanks to 0x7375646F for the power profile method usage: https://github.com/0x7375646F/Linuwu-Sense/blob/73a25ec243a44ba2b1703e8d0a76fa2735062506/src/linuwu_sense.c
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

ssize_t nitro_profile_write(
    struct file* file,
    const char __user* buf,
    size_t count,
    loff_t* ppos
) {
    // thanks to 0x7375646F for the power profile method usage: https://github.com/0x7375646F/Linuwu-Sense/blob/73a25ec243a44ba2b1703e8d0a76fa2735062506/src/linuwu_sense.c
    char profile[11];
    if(copy_from_user(profile, buf, 11)) {
        return -EFAULT;
    }
    struct profile_write_in write_in = {
        .gmInput = 1
    };

    if(init_str_equal(profile, "quiet", 5)) {
        write_in.gmInput = 0;
    }
    else if(init_str_equal(profile, "balanced", 8)) {
        write_in.gmInput = 1;
    }
    else if(init_str_equal(profile, "performance", 11)) {
        write_in.gmInput = 4;
    }
    else if(init_str_equal(profile, "eco", 3)) {
        write_in.gmInput = 6;
    }
    else {
        printk(KERN_WARNING "Undefined input for setting Nitro power profile: %s", profile);
        return -EINVAL;
    }
    write_in.gmInput = write_in.gmInput << 8;
    write_in.gmInput |= 0x0B;

    struct wmi_method_input set_platform_profile = {
        .in = { sizeof(struct profile_write_in), &write_in },
        .instance = 0,
        .method_id = POWER_PROFILE_SET_MISCELLANEOUS_SETTING_METHOD_ID
    };
    if(down_interruptible(&nitro_profile_lock)) return -ERESTARTSYS;
    union acpi_object* obj = run_wmi_command(nitro_profile_char_dev.wdev, &set_platform_profile, sizeof(struct profile_write_out), "Set gaming profile");
    up(&nitro_profile_lock);
    // ignore output
    if(obj) {
        kfree(obj);
        return count;
    }
    return -EFAULT;
}
