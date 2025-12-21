#ifndef NITRO_ANV15_51_BATTERY_CONTROL_H
#define NITRO_ANV15_51_BATTERY_CONTROL_H

#include "misc.h"

#include <linux/fs.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/wmi.h>


#define BATTERY_CONTROL_GUID "79772EC5-04B1-4bfd-843C-61E7F77B6CC9"


/************************************
* File operation method definitions *
************************************/

ssize_t nitro_battery_read(
    struct file* file,
    char __user* buf,
    size_t count,
    loff_t* ppos
);

ssize_t nitro_battery_write(
    struct file* file,
    const char __user* buf,
    size_t count,
    loff_t* ppos
);

int nitro_battery_open(struct inode* inode, struct file* file);

int nitro_battery_release(struct inode* node, struct file* file);


/************************************
*** Device and utility structures ***
************************************/

int batt_probe(struct wmi_device *wdev, const void *context);

void batt_remove(struct wmi_device *wdev);


/************************************
******* WMI evaluation in/out *******
************************************/

struct __attribute__((packed)) battery_get_charge_limit_in {
    u8 uBatteryNo;
    u8 uFunctionQuery;
    u8 uReserved[2];
};

struct __attribute__((packed)) battery_get_charge_limit_out {
    u8 uFunctionList;
    u8 uReturn[2];
    u8 uFunctionStatus[5];
};

struct __attribute__((packed)) battery_set_charge_limit_in {
    u8 uBatteryNo;
    u8 uFunctionMask;
    u8 uFunctionStatus;
    u8 uReservedIn[5];
};

struct __attribute__((packed)) battery_set_charge_limit_out {
    u16 uReturn;
    u16 uReservedOut;
};

#endif
