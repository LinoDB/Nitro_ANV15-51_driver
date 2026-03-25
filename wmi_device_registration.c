#include "misc.h"

#include <linux/wmi.h>


extern struct nitro_char_dev nitro_battery_char_dev;
extern struct nitro_char_dev nitro_profile_char_dev;
extern struct nitro_char_dev nitro_fan_char_dev;


/************************************
********* WMI setup methods *********
************************************/

int battery_wmi_device_probe(struct wmi_device *wdev, const void *context) {
    if(!wmi_has_guid(BATTERY_WMI_DEVICE_GUID)) {
        printk(KERN_ERR "Nitro Battery WMI Device init error: WMI device GUID couldn't be found");
        return -ENODEV;
    }
    nitro_battery_char_dev.wdev = wdev;
    return 0;
}

void battery_wmi_device_remove(struct wmi_device *wdev) {
    if(nitro_battery_char_dev.wdev) nitro_battery_char_dev.wdev = NULL;
}


int gaming_wmi_device_probe(struct wmi_device *wdev, const void *context) {
    if(!wmi_has_guid(GAMING_WMI_DEVICE_GUID)) {
        printk(KERN_ERR "Nitro Gaming WMI Device init error: WMI device GUID couldn't be found");
        return -ENODEV;
    }
    nitro_profile_char_dev.wdev = wdev;
    nitro_fan_char_dev.wdev = wdev;
    return 0;
}

void gaming_wmi_device_remove(struct wmi_device *wdev) {
    if(nitro_profile_char_dev.wdev) nitro_profile_char_dev.wdev = NULL;
    if(nitro_fan_char_dev.wdev) nitro_fan_char_dev.wdev = NULL;
}
