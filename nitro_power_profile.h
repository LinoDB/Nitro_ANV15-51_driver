#ifndef NITRO_ANV15_51_POWER_PROFILES_H
#define NITRO_ANV15_51_POWER_PROFILES_H

#include "misc.h"

#include <linux/fs.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/wmi.h>


#define POWER_PROFILE_GUID "7A4DDFE7-5B5D-40B4-8595-4408E0CC7F56"


/************************************
* File operation method definitions *
************************************/

ssize_t nitro_profile_read(
    struct file* file,
    char __user* buf,
    size_t count,
    loff_t* ppos
);

ssize_t nitro_profile_write(
    struct file* file,
    const char __user* buf,
    size_t count,
    loff_t* ppos
);


/************************************
*** Device and utility structures ***
************************************/

int prof_probe(struct wmi_device *wdev, const void *context);

void prof_remove(struct wmi_device *wdev);


/************************************
******* WMI evaluation in/out *******
************************************/

struct __attribute__((packed)) profile_get_in {
    u32 gmInput;
};

struct __attribute__((packed)) profile_get_out {
    u64 gmOutput;
};

struct __attribute__((packed)) profile_write_in {
    u64 gmInput;
};

struct __attribute__((packed)) profile_write_out {
    u32 gmOutput;
};

#endif
