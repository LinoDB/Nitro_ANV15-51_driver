#include "nitro_fan_control.h"

#include <linux/acpi.h>
#include <linux/errno.h>
#include <linux/fs.h>
#include <linux/kdev_t.h>
#include <linux/kernel.h>
#include <linux/semaphore.h>
#include <linux/wmi.h>


/************************************
******* WMI evaluation in/out *******
************************************/

struct get_fan_behaviour_in check_fan_behaviour_in = {
    .gmInput = 9
};

const struct wmi_method_input read_fan_behaviour = {
    .in = { sizeof(struct get_fan_behaviour_in), &check_fan_behaviour_in },
    .instance = 0,
    .method_id = FAN_GET_BEHAVIOUR_METHOD_ID
};


/************************************
********* Device management *********
************************************/

struct file_operations cpu_fan_fops = {
    .owner = THIS_MODULE,
    .read = nitro_fan_read,
    // .write = nitro_fan_write,
    .open = nitro_open,
    .release = nitro_release
};

struct file_operations gpu_fan_fops = {
    .owner = THIS_MODULE,
    .read = nitro_fan_read,
    // .write = nitro_fan_write,
    .open = nitro_open,
    .release = nitro_release
};


extern struct wmi_driver gaming_driver;
extern struct semaphore gaming_semaphore;


struct nitro_char_dev nitro_cpu_fan_char_dev = {
    .fops = &cpu_fan_fops,
    .driver = &gaming_driver,
    .name = "CPU Fan Controller",
    .file_name = "nitro_anv15_51!fan_control_cpu",
    .initialized = false,
    .semaphore = &gaming_semaphore,
    .additional_data = 2,
};

struct nitro_char_dev nitro_gpu_fan_char_dev = {
    .fops = &gpu_fan_fops,
    .driver = &gaming_driver,
    .name = "GPU Fan Controller",
    .file_name = "nitro_anv15_51!fan_control_gpu",
    .initialized = false,
    .semaphore = &gaming_semaphore,
    .additional_data = 2,
};


/************************************
****** File operation methods *******
************************************/

int _nitro_behaviour_check(struct wmi_device* wdev, u64* behaviour) {
    union acpi_object* obj = run_wmi_command(wdev, &read_fan_behaviour, sizeof(struct get_fan_behaviour_out), "Read fan behaviour");
    if(obj) {
        *behaviour = ((struct get_fan_behaviour_out*)obj->buffer.pointer)->gmOutput >> 8;
        kfree(obj);
    }
    else {
        return -EFAULT;
    }
    return 0;
}

ssize_t nitro_fan_read(
    struct file* file,
    char __user* buf,
    size_t count,
    loff_t* ppos
) {
    // thanks to 0x7375646F for the fan method usage: https://github.com/0x7375646F/Linuwu-Sense/blob/73a25ec243a44ba2b1703e8d0a76fa2735062506/src/linuwu_sense.c
    if(*ppos > 0) return 0;
    struct nitro_char_dev* char_dev = file->private_data;
    u64 behaviour;
    int err = _nitro_behaviour_check(char_dev->wdev, &behaviour);
    if(err) {
        printk(KERN_ERR "Failed to read Nitro fan mode");
        return err;
    }

    bool cpu = char_dev->name[0] == 'C';
    struct get_fan_speed_in check_fan_speed_in = {
        .gmInput = cpu ? CPU_FAN_SPEED_READ_VALUE : GPU_FAN_SPEED_READ_VALUE
    };
    const struct wmi_method_input read_fan_speed = {
        .in = { sizeof(struct get_fan_speed_in), &check_fan_speed_in },
        .instance = 0,
        .method_id = FAN_GET_SPEED_METHOD_ID
    };

    u64 speed;
    union acpi_object* obj = run_wmi_command(char_dev->wdev, &read_fan_speed, sizeof(struct get_fan_speed_out), "Read fan speed");
    if(obj) {
        speed = ((struct get_fan_speed_out*)obj->buffer.pointer)->gmOutput >> 8;
        kfree(obj);
    }
    else {
        printk(KERN_ERR "Failed to read Nitro fan speed");
        return -EFAULT;
    }

    char* mode = behaviour & (cpu ? CPU_FAN_BEHAVIOUR_MASK : GPU_FAN_BEHAVIOUR_MASK) ? "manual" : "auto";
    char tmp_buf[30]; // max 29
    size_t actual_count = sprintf(tmp_buf, "Mode: %s\nSpeed: %llu RPM\n", mode, speed);
    if(copy_to_user(buf, tmp_buf, actual_count)) {
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
//     // thanks to 0x7375646F for the fan method usage: https://github.com/0x7375646F/Linuwu-Sense/blob/73a25ec243a44ba2b1703e8d0a76fa2735062506/src/linuwu_sense.c
//     struct nitro_char_dev* char_dev = file->private_data;
//     char fan_input[5];
//     if(copy_from_user(fan_input, buf, 5)) {
//         return -EFAULT;
//     }
//     bool cpu = char_dev->name[0] == 'C';

//     struct set_fan_behaviour_in fan_behaviour_in = {
//         .gmInput = 0
//     };

//     if(init_str_equal(fan_input, "reset", 5)) {
//         fan_behaviour_in.gmInput = cpu ? CPU_FAN_AUTO_MODE : GPU_FAN_AUTO_MODE;
//     }
//     else if(init_str_equal(fan_input, "auto", 4)) {
//         fan_behaviour_in.gmInput = 
//         // set auto
//     }
    
//     const struct wmi_method_input set_fan_mode = {
//         .in = { sizeof(struct set_fan_behaviour_in), &fan_behaviour_in },
//         .instance = 0,
//         .method_id = FAN_SET_BEHAVIOUR_METHOD_ID
//     };

    

//     switch(*activate) {
//         case '0':
//             set_charge_limit_in.uFunctionStatus = 0;
//             break;
//         case '1':
//             set_charge_limit_in.uFunctionStatus = 1;
//             break;
//         default:
//             printk(KERN_WARNING "Undefined input for setting Nitro battery control mode: %s", activate);
//             return -EINVAL;
//     }
//     struct wmi_method_input write_battery_charge_limited = {
//         .in = { sizeof(struct battery_set_charge_limit_in), &set_charge_limit_in },
//         .instance = 0,
//         .method_id = BATTERY_SET_HEALTH_CONTROL_METHOD_ID
//     };
//     if(down_interruptible(nitro_battery_char_dev.semaphore)) return -ERESTARTSYS;
//     union acpi_object* obj = run_wmi_command(nitro_battery_char_dev.wdev, &write_battery_charge_limited, sizeof(struct battery_set_charge_limit_out), "Set battery charge limit");
//     up(nitro_battery_char_dev.semaphore);
//     // ignore output
//     if(obj) {
//         kfree(obj);
//         return count;
//     }
//     return -EFAULT;
// }
