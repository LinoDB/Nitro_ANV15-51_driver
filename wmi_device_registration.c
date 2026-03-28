#include "misc.h"
#include "nitro_fan_control.h"

#include <linux/wmi.h>


extern struct nitro_char_dev nitro_battery_char_dev;
extern struct nitro_char_dev nitro_profile_char_dev;
extern struct nitro_char_dev nitro_cpu_fan_char_dev;
extern struct nitro_char_dev nitro_gpu_fan_char_dev;


/************************************
********* WMI setup methods *********
************************************/

int battery_wmi_device_probe(struct wmi_device* wdev, const void* context) {
    if(!wmi_has_guid(BATTERY_WMI_DEVICE_GUID)) {
        printk(KERN_ERR "Nitro Battery WMI Device init error: WMI device GUID couldn't be found");
        return -ENODEV;
    }
    nitro_battery_char_dev.wdev = wdev;
    return 0;
}

void battery_wmi_device_remove(struct wmi_device* wdev) {
    if(nitro_battery_char_dev.wdev) nitro_battery_char_dev.wdev = NULL;
}


int gaming_wmi_device_probe(struct wmi_device* wdev, const void* context) {
    if(!wmi_has_guid(GAMING_WMI_DEVICE_GUID)) {
        printk(KERN_ERR "Nitro Gaming WMI Device init error: WMI device GUID couldn't be found");
        return -ENODEV;
    }
    u64 behaviour;
    int err = _nitro_behaviour_check(wdev, &behaviour);
    if(err) {
        printk(KERN_ERR "Nitro WMI gaming setup error: Couldn't read fan behaviour");
        return err;
    }
    nitro_cpu_fan_char_dev.additional_data = behaviour & CPU_FAN_BEHAVIOUR_MASK ? 0 : 1;
    nitro_gpu_fan_char_dev.additional_data = behaviour & GPU_FAN_BEHAVIOUR_MASK ? 0 : 1;

    nitro_profile_char_dev.wdev = wdev;
    nitro_cpu_fan_char_dev.wdev = wdev;
    nitro_gpu_fan_char_dev.wdev = wdev;
    return 0;
}

void gaming_wmi_device_remove(struct wmi_device* wdev) {
    if(nitro_profile_char_dev.wdev) nitro_profile_char_dev.wdev = NULL;
    if(nitro_cpu_fan_char_dev.wdev) nitro_cpu_fan_char_dev.wdev = NULL;
    if(nitro_gpu_fan_char_dev.wdev) nitro_gpu_fan_char_dev.wdev = NULL;
}
