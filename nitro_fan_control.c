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

struct file_operations fan_fops = {
    .owner = THIS_MODULE,
    .read = nitro_fan_read,
    // .write = nitro_fan_write,
    .open = nitro_open,
    .release = nitro_release
};

extern struct wmi_driver gaming_driver;

struct nitro_char_dev nitro_fan_char_dev = {
    .fops = &fan_fops,
    .driver = &gaming_driver,
    .name = "Fan Controller",
    .file_name = "nitro_anv15_51!fan_control",
    .initialized = false,
};


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
    union acpi_object* b_obj = run_wmi_command(nitro_fan_char_dev.wdev, &read_fan_behaviour, sizeof(struct get_fan_behaviour_out), "Read fan behaviour");
    if(b_obj) {
        behaviour = ((struct get_fan_behaviour_out*)b_obj->buffer.pointer)->gmOutput >> 8;
        kfree(b_obj);
    }
    else {
        printk(KERN_INFO "Failed to read Nitro fan mode");
        return -EFAULT;
    }

    struct get_fan_speed_in check_fan_speed_in = {
        .gmInput = CPU_FAN_SPEED_READ_VALUE
    };
    const struct wmi_method_input read_fan_speed = {
        .in = { sizeof(struct get_fan_speed_in), &check_fan_speed_in },
        .instance = 0,
        .method_id = FAN_GET_SPEED_METHOD_ID
    };

    u64 speed[2];
    for(int i = 0; i < 2; i++) {
        union acpi_object* s_obj = run_wmi_command(nitro_fan_char_dev.wdev, &read_fan_speed, sizeof(struct get_fan_speed_out), "Read fan speed");
        if(s_obj) {
            speed[i] = ((struct get_fan_speed_out*)s_obj->buffer.pointer)->gmOutput >> 8;
            kfree(s_obj);
        }
        else {
            printk(KERN_INFO "Failed to read Nitro fan speed");
            return -EFAULT;
        }
        check_fan_speed_in.gmInput = GPU_FAN_SPEED_READ_VALUE;
    }

    char* cpu_auto = behaviour & 0x02 ? "manual" : "auto";
    char* gpu_auto = behaviour & 0x90 ? "manual" : "auto";

    char tmp_buf[45]; // max 44
    size_t actual_count = sprintf(tmp_buf, "CPU: %s, %llu RPM\nGPU: %s, %llu RPM\n", cpu_auto, speed[0], gpu_auto, speed[1]);
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
